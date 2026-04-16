# surface consumer listener 深挖记录（2026-04-16）

> 目标：继续确认当前板子的 surface 崩溃到底是不是 sample 自己的 listener 逻辑导致，还是板端 `libsurface` consumer listener dispatch 路径本身就有问题。

## 1. 对照的 listener 到底差多少

### 1.1 官方 `hcodec_demo_arm` 里的 listener

官方 helper `TesterCommon::Listener::OnBufferAvailable()`：

```cpp
void TesterCommon::Listener::OnBufferAvailable()
{
    sptr<SurfaceBuffer> buffer;
    int32_t fence;
    int64_t timestamp;
    OHOS::Rect damage;
    GSError err = tester_->surface_->AcquireBuffer(buffer, fence, timestamp, damage);
    if (err != GSERROR_OK || buffer == nullptr) {
        TLOGW("AcquireBuffer failed");
        return;
    }
    tester_->surface_->ReleaseBuffer(buffer, -1);
}
```

特点：

- `sptr<IBufferConsumerListener>` 路径注册
- `surface_` 保存在 `TesterCommon` 成员里
- callback 里只做最小 `AcquireBuffer + ReleaseBuffer`
- 没有复杂业务处理

### 1.2 我们自己的 `hcodec_minidec` listener

`hcodec_minidec` 的 surface listener 虽然做过几轮试验，但核心正常版的行为与官方非常接近：

- `sptr<IBufferConsumerListener>` 路径注册
- callback 里本质也是 `AcquireBuffer + ReleaseBuffer`
- 额外差异主要只是：
  - 是否打日志
  - `consumer_` 是 listener 自己持有，还是通过 `tester_->surface_` 间接访问
  - callback 里是否做计数

### 1.3 第一层结论

这两边的 listener 主体并没有本质差别，至少没有差到能解释：

- 一个会稳定 crash
- 另一个完全不 crash

因为实际上它们 **都 crash** 了，而且 fault stack 非常接近。

---

## 2. 已做过的 listener 级排除实验

### 2.1 `hcodec_minidec` listener 改成 no-op

把 `OnBufferAvailable()` 改成：

- 只计数
- 不 `AcquireBuffer`
- 不 `ReleaseBuffer`
- 不写日志内容之外的任何东西

结果：

- 仍然 `Signal 11`
- 而且连我们自己的 `[surface]` callback 日志都来不及打印

说明：

> crash 不是简单由 `AcquireBuffer/ReleaseBuffer` 的具体逻辑触发，更像是在 `libsurface` 调 listener 回调的派发阶段就已经出问题。

### 2.2 改走 `IBufferConsumerListenerClazz + RefBase`

尝试改用 raw listener 注册路径：

- `IBufferConsumerListenerClazz`
- `RefBase`
- `RegisterConsumerListener(listener.GetRefPtr())`

结果：

- 更早崩
- 在 `create output surface` 阶段就 `Signal 11`

说明：

> raw-listener 注册路径不是 workaround，至少在当前板端环境下更差。

---

## 3. 不经过 codec 的最小 pure surface probe

为了排除“媒体 codec 输出链路”这个变量，又专门做了一个完全不经过 codec 的纯 surface probe：

- 新增源码：
  - `manual-arm-build/surface_listener_probe.cpp`
- 新增构建脚本：
  - `manual-arm-build/build_surface_listener_probe_arm.sh`
- 产物：
  - `/data/local/tmp/surface_listener_probe_arm`

这个 probe 做的事非常简单：

1. `CreateSurfaceAsConsumer()`
2. `RegisterConsumerListener()`
3. `CreateSurfaceAsProducer()`
4. `RequestBuffer()`
5. `FlushBuffer()`

并且先跑的是 **noop listener**，即 callback 体不做任何 `AcquireBuffer/ReleaseBuffer`。

### 3.1 运行日志

```text
[probe] create consumer
[probe] RegisterConsumerListener ret=0
[probe] SetDefaultUsage ret=0
[probe] SetQueueSize ret=40001000
[probe] RequestBuffer ret=0 buffer=0xf7462e60 fence=-1
[probe] FlushBuffer begin
Signal 11
```

### 3.2 fault log

- `/home/neardws/openharmony-build/logs/hcodec/cppcrash-surface_listener_probe_arm-0-20260416160404`

关键栈：

```text
Reason: Signal:SIGSEGV(SEGV_ACCERR)
Process name:/data/local/tmp/surface_listener_probe_arm
Fault thread info:
Tid:13810, Name:surface_listene
#01 libsurface.z.so(OHOS::BufferQueue::CallConsumerListener()+104)
#02 libsurface.z.so(OHOS::BufferQueue::FlushBuffer(...)+760)
#03 libsurface.z.so(OHOS::BufferQueueProducer::FlushBuffer(...)+68)
#04 libsurface.z.so(OHOS::ProducerSurface::FlushBuffer(...)+150)
#05 libsurface.z.so(OHOS::ProducerSurface::FlushBuffer(...)+80)
#06 libsurface.z.so(OHOS::ProducerSurface::FlushBuffer(...)+58)
#07 /data/local/tmp/surface_listener_probe_arm(main+770)
```

### 3.3 这条证据的意义

这条 probe 非常关键，因为它证明了：

> 即使完全不经过 codec，单纯是 `surface producer -> FlushBuffer -> consumer listener dispatch` 这条最小路径，在当前板子上也会 crash。

这就把问题范围进一步缩小到了：

- `libsurface`
- `BufferQueue::CallConsumerListener()`
- consumer listener dispatch / callback 调用路径

而不是：

- `hcodec_minidec` 自己的 drain 逻辑
- 官方 `hcodec_demo` 自己的 sample 流程
- AVCodec/HCodec 专属行为

---

## 4. 三条证据并起来看

当前已经有 3 条独立证据都指向同一层：

1. **`hcodec_minidec_arm` surface crash**
   - `libsurface.z.so -> BufferQueue::CallConsumerListener()`
2. **官方 `hcodec_demo_arm(apiType=1)` surface crash**
   - `libsurface.z.so -> BufferQueue::CallConsumerListener()`
3. **pure surface probe noop listener crash**
   - `libsurface.z.so -> BufferQueue::CallConsumerListener()`

这三条的共同点是：

- 都不是同一个 sample
- 都不是同一个上层 API 路径
- 甚至第 3 条已经彻底绕开 codec
- 但最终都落到同一个 `libsurface` 位置

---

## 5. 当前最稳结论

一句话总结：

> 当前板子的 surface 问题已经被进一步收敛成一个更底层的系统问题：只要走到 `libsurface` 的 consumer listener dispatch 路径，哪怕是最小的 pure surface producer/consumer + noop listener，也会在 `BufferQueue::CallConsumerListener()` 附近崩溃，因此问题不再像是媒体 sample 逻辑 bug，而更像板端 `libsurface` / 图形栈回调派发路径本身的系统级缺陷或 ABI/运行时兼容问题。
