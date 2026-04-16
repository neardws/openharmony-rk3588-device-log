# `hcodec_demo_arm` vs `hcodec_minidec` surface crash 并排对照（2026-04-16）

> 目标：把两条已经在板子上真实跑过的 surface crash 证据并排摆出来，判断它们到底是“同一个系统级问题”还是“两个不同问题刚好都炸了”。

## 1. 对照范围

本对照只比较这两条已经真实复现的 crash：

1. **自定义 `hcodec_minidec_arm` surface crash**
2. **官方 `hcodec_demo_arm` `apiType=1` surface crash**

不再把早期那个 `SetOutputSurface failed: 63570434` 混进来，因为那一层已经被定位为 `AVCS_ERR_INVALID_STATE` + 调用时序问题。

---

## 2. 并排对照表

| 维度 | `hcodec_minidec_arm` surface crash | 官方 `hcodec_demo_arm` `apiType=1` surface crash |
|---|---|---|
| binary | `/data/local/tmp/hcodec_minidec_arm` | `/data/local/tmp/hcodec_demo_arm` |
| 来源 | 自定义最小 sample，基于 inner API | 官方 `hcodec_demo.cpp` + helper 手工重编为 32-bit |
| API 路径 | inner API / `AVCodecVideoDecoder` | C API new path / `apiType=1` |
| 输入样本 | `/data/local/tmp/out_320_240_10s.h264`（surface smoke） | `/data/local/tmp/out_320_240_10s.h264` |
| 运行模式 | surface | surface |
| 关键前提 | 已修正调用顺序为 `Configure -> SetOutputSurface -> Prepare -> Start` | `apiType=1` buffer mode 已先证明能稳定跑通 |
| 崩溃前推进程度 | 已走到 `configure -> create output surface -> set output surface -> prepare -> start -> first input callback -> first output callback` | 已进入官方 sample surface mode，连续喂入前几帧样本后崩溃 |
| 用户态最后可见日志 | `[callback] first input ...` / `[callback] first output idx=5 size=0 flag=0x0` / `Signal 11` | `api type=1, decoder, surface mode`，随后多次 `GetNextSample` / `BeforeQueueInput`，然后 `Signal 11` |
| faultlog | `logs/hcodec/cppcrash-hcodec_minidec_arm-0-20260416124430` | `logs/hcodec/cppcrash-hcodec_demo_arm-0-20260416152117` |
| `Process name` | `/data/local/tmp/hcodec_minidec_arm` | `/data/local/tmp/hcodec_demo_arm` |
| `Reason` | `SIGSEGV(SEGV_ACCERR)@0xf76d78f0` | `SIGSEGV(SEGV_MAPERR)@0x726f6c6e` |
| `Fault thread info` | `Tid:8558, Name:OS_IPC_0_8558` | `Tid:12798, Name:OS_IPC_2_12798` |
| Top stack #1 | `libsurface.z.so(OHOS::BufferQueue::CallConsumerListener()+104)` | `libsurface.z.so(OHOS::BufferQueue::CallConsumerListener()+104)` |
| Top stack #2 | `libsurface.z.so(OHOS::BufferQueue::FlushBuffer(...)+760)` | `libsurface.z.so(OHOS::BufferQueue::FlushBuffer(...)+760)` |
| Top stack #3 | `libsurface.z.so(OHOS::BufferQueueProducer::FlushBuffer(...)+68)` | `libsurface.z.so(OHOS::BufferQueueProducer::FlushBuffer(...)+68)` |
| 是否落在 surface callback/dispatch 路径 | 是 | 是 |
| 是否是 sample 自己独有的 helper 逻辑导致 | 不能单独解释 | 不能单独解释 |
| 对问题归因的意义 | 自定义 sample 一旦真正走到 surface 输出，就会在 `CallConsumerListener()` 附近崩 | 连官方 sample 的稳定 `apiType=1` 路也会在同一条 `CallConsumerListener()` 路上崩 |

---

## 3. 关键信息拆解

### 3.1 共同点，比差异更重要

两边虽然：

- API 层不一样
- sample 来源不一样
- fault thread 编号不一样
- fault address 不一样
- `SEGV_ACCERR` / `SEGV_MAPERR` 细节不一样

但它们有一个最关键的共同点：

```text
libsurface.z.so
  -> OHOS::BufferQueue::CallConsumerListener()
  -> OHOS::BufferQueue::FlushBuffer(...)
  -> OHOS::BufferQueueProducer::FlushBuffer(...)
```

这不是“都用了 surface，所以随便撞上同一个库名”那么简单，而是：

> 两条完全不同的上层 sample，最终都在 surface consumer listener 回调派发这一层附近出事。

### 3.2 两边分别排除了什么

#### `hcodec_minidec_arm`

已经排除过：

- `SetOutputSurface` 调用太早导致的 `AVCS_ERR_INVALID_STATE`
- listener 里 `AcquireBuffer/ReleaseBuffer` 逻辑直接导致崩溃
- 简单的手工编译参数差异导致的 surface 崩溃

#### 官方 `hcodec_demo_arm` `apiType=1`

已经排除过：

- 自定义 `hcodec_minidec` 私有实现路径
- internal codecbase helper 的不稳定性（因为这里用的是 `apiType=1`，而不是 `apiType=0`）
- “只有我们自定义 sample 才会 surface 崩”的解释

### 3.3 这张对照表真正说明了什么

这张表要证明的不是：

- 两个 crash 100% 完全同根同因

而是：

> “当前板子的 surface 输出问题已经被两个独立 sample 路径交叉复现，并且都指向 `libsurface` 的 consumer listener dispatch 路径，因此它非常像板端 surface callback/dispatch 路径的系统级问题，而不是某一个单独 sample 的业务逻辑 bug。”

---

## 4. 当前最稳结论

一句话版：

> `hcodec_minidec_arm` 和官方重编后的 `hcodec_demo_arm(apiType=1)`，只要真正走到 surface 输出，都会在 `libsurface.z.so -> BufferQueue::CallConsumerListener()` 这一层附近崩溃，所以“surface crash 是 `hcodec_minidec` 独有 bug”这条解释已经基本站不住了。

---

## 5. 下一步建议

优先级建议：

1. **继续从 `libsurface` / consumer listener dispatch 路径取更多板端证据**
   - 比如更细的 faultlog、同版本其他 surface consumer 用例、是否任何 app-side consumer listener 都会炸
2. **找板端或系统自带的更底层 surface consumer 用例做对照**
   - 目标是确认这是“所有 app-side consumer listener 都崩”，还是“只在媒体 codec 输出接到 surface consumer 时崩”
3. **不要再把重点放在 `hcodec_minidec` 自己的 drain 逻辑上**
   - 因为官方 sample 已经把“自定义 sample 私有 bug”这个解释压得很弱了
