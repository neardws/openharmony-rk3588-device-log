# 官方 `hcodec_demo` 32-bit 手工重编与板端验证（2026-04-16）

## 1. 背景

当前板子用户态仍是 32-bit musl，因此直接使用 `out/rk3588` 产出的 64-bit `hcodec_demo` 不可行。

同时，现成的 `out/rk3568/multimedia/av_codec/hcodec_demo` 虽然是 32-bit ARM，但直接下发到当前板子后无法运行，原因不是文件损坏，而是运行时 ABI / 系统库版本不匹配：

- 缺少旧版 `libsurface.z.so` 的符号：
  - `OHOS::Surface::CreateSurfaceAsConsumer(std::string)`
- 缺少旧版 `libhcodec.z.so` 的调试辅助符号：
  - `OHOS::MediaAVCodec::StringifyMeta(std::shared_ptr<Meta>&)`

因此，本轮改为：

> 按当前板子可运行的 32-bit 路线，手工重编官方 `hcodec_demo.cpp` 及其 helper。

## 2. 手工重编方案

### 2.1 源码

使用官方源码：

- `foundation/multimedia/av_codec/test/unittest/hcodec_test/demo/hcodec_demo.cpp`

以及 helper：

- `command_parser.cpp`
- `start_code_detector.cpp`
- `test_utils.cpp`
- `tester_capi.cpp`
- `tester_codecbase.cpp`
- `tester_common.cpp`

### 2.2 构建脚本

新增脚本：

- `manual-arm-build/build_hcodec_demo_arm.sh`

另加一个仅用于补齐当前板端 `libhcodec.z.so` 未导出的调试函数 shim：

- `manual-arm-build/hcodec_stringify_meta_shim.cpp`

这个 shim 不改变 sample 主逻辑，只是补齐：

- `StringifyMeta(std::shared_ptr<Meta>&)`

### 2.3 链接策略

手工构建时，优先使用当前板子兼容的 32-bit 路线：

- 头文件来自当前源码树
- 一部分链接库来自当前板子拉回的 system libs：
  - `libhcodec.z.so`
  - `libnative_media_codecbase.so`
  - `libnative_media_vdec.so`
  - `libnative_media_venc.so`
- 另一部分继续使用已验证可和当前板子配合工作的 devlibs：
  - `libav_codec_client.z.so`
  - `libmedia_foundation.z.so`
  - `libnative_media_core.so`
  - `libsurface.z.so`
  - `libutils.z.so`
  - `libsec_shared.z.so`
  - `libhilog.so`
  - `libipc_single.z.so`

### 2.4 产物

编译成功产物：

- `/home/neardws/openharmony-build/manual-arm-build-hcodec-demo/hcodec_demo.arm`

本地校验：

```text
ELF 32-bit LSB pie executable, ARM, interpreter /lib/ld-musl-arm.so.1
```

## 3. 下发到板子

已下发为：

- `/data/local/tmp/hcodec_demo_arm`

运行空参数时，binary 本身已能启动并进入 sample 主逻辑，不再出现旧版 `hcodec_demo` 那种 relocation error。

## 4. 板端验证结果

## 4.1 `apiType=0`（codecbase 路）

直接运行会很早崩溃，fault log 显示：

- `cppcrash-hcodec_demo_arm-0-20260416151628`
- crash 点在：
  - `OHOS::MediaAVCodec::GetCodecName(...)`

这说明：

> 即使 binary 已能运行，官方 `codecbase` 路仍然依赖板端内部 hcodec capability/helper ABI，而这部分在当前镜像上并不稳定。

## 4.2 `apiType=1` + buffer mode

命令：

```bash
/data/local/tmp/hcodec_demo_arm \
  --in /data/local/tmp/out_320_240_10s.h264 \
  --width 320 \
  --height 240 \
  --protocol 0 \
  --isEncoder 0 \
  --isBufferMode 1 \
  --apiType 1 \
  --frameRate 30 \
  --timeout 30000
```

结果：**成功跑完到 EOS**。

日志尾部显示：

```text
I: [AfterGotOutput] decoder output ... out fps 247.68
I: [AfterGotOutput] decoder output: flags=0x1 (eos)
I: [WaitForOutput] output eos, quit loop
```

并打印了稳定的 API 耗时统计。

这说明：

> 重编后的官方 sample 在 `apiType=1` 的 buffer 路上，已经能在当前板子上正常工作。

## 4.3 `apiType=1` + buffer mode（1080p60 / 10s）

命令：

```bash
/data/local/tmp/hcodec_demo_arm \
  --in /data/local/tmp/test-real-lauterbrunnen-1080p60-10s-copy.h264 \
  --width 1920 \
  --height 1080 \
  --protocol 0 \
  --isEncoder 0 \
  --isBufferMode 1 \
  --apiType 1 \
  --frameRate 60 \
  --timeout 120000
```

结果：**成功跑完到 EOS**。

日志尾部显示：

```text
... in fps 125.46, out fps 123.40
I: [AfterGotOutput] decoder output: flags=0x1 (eos)
I: [WaitForOutput] output eos, quit loop
```

这说明：

> 重编后的官方 sample 在 `apiType=1` buffer 路上，面对 `1080p60 / 10s` 样本时，吞吐已经明显高于 real-time 60 fps。

## 4.4 `apiType=1` + buffer mode（1080p60 / 10m18s）

命令：

```bash
/data/local/tmp/hcodec_demo_arm \
  --in /data/local/tmp/test-h264-real-lauterbrunnen-1920x1080-high-60fps-10m18s.h264 \
  --width 1920 \
  --height 1080 \
  --protocol 0 \
  --isEncoder 0 \
  --isBufferMode 1 \
  --apiType 1 \
  --frameRate 60 \
  --timeout 600000
```

注意：最初曾误用 `--timeout 0`，导致 sample 进入反复 `WaitForInput time out` 的异常状态。改成大的正超时后，长跑可以正常完成。

结果：**成功跑完整段长视频并到 EOS**。

日志尾部显示：

```text
... in fps 123.20, out fps 123.17
I: [AfterGotOutput] decoder output: flags=0x1 (eos)
I: [WaitForOutput] output eos, quit loop
```

这说明：

> 官方 `apiType=1` buffer 路不仅能跑短样本，而且长时间稳定吞吐也维持在大约 `123 fps`，没有出现我们自定义 sample 那种明显掉到 `20 fps` 的情况。

## 4.5 `apiType=1` + buffer mode（2688x1520 / 60fps / 10s）

命令：

```bash
/data/local/tmp/hcodec_demo_arm \
  --in /data/local/tmp/test-h264-2688x1520-main-60fps-10s.h264 \
  --width 2688 \
  --height 1520 \
  --protocol 0 \
  --isEncoder 0 \
  --isBufferMode 1 \
  --apiType 1 \
  --frameRate 60 \
  --timeout 180000
```

结果：**成功跑完到 EOS**。

日志尾部显示：

```text
... in fps 83.31, out fps 82.10
I: [AfterGotOutput] decoder output: flags=0x1 (eos)
I: [WaitForOutput] output eos, quit loop
```

这说明：

> 在更接近 IPC 主码流的 `2688x1520@60` 样本上，官方 sample 的 `apiType=1` buffer 路仍然能达到约 `82 fps`，同样高于 real-time 60 fps。

## 4.6 与 `hcodec_minidec` 的直接对照

对照同一天已经测过的 `hcodec_minidec_arm`：

- `1080p60 / 10s`
  - `hcodec_minidec_arm`: 约 `20 fps`
  - 官方 `hcodec_demo_arm(apiType=1, buffer)`: 约 `123 fps`
- `1080p60 / 10m18s`
  - `hcodec_minidec_arm` 早先长跑结论：约 `20 fps`
  - 官方 `hcodec_demo_arm(apiType=1, buffer)`: 约 `123 fps`
- `2688x1520@60 / 10s`
  - `hcodec_minidec_arm`: 约 `10 fps`
  - 官方 `hcodec_demo_arm(apiType=1, buffer)`: 约 `82 fps`

这个差距非常大，已经足够说明：

> 当前从“码流 -> buffer 输出”这段，`hcodec_minidec_arm` 自己的 sample/pipeline 实现就是主要瓶颈之一，至少远没有逼近当前板子的 buffer-mode 硬解输出上限。

## 4.7 `apiType=1` + surface mode

命令：

```bash
/data/local/tmp/hcodec_demo_arm \
  --in /data/local/tmp/out_320_240_10s.h264 \
  --width 320 \
  --height 240 \
  --protocol 0 \
  --isEncoder 0 \
  --isBufferMode 0 \
  --apiType 1 \
  --frameRate 30 \
  --timeout 30000
```

结果：**崩溃**。

运行日志在 sample 读入前几帧后出现：

```text
Signal 11
```

对应 fault log：

- `/home/neardws/openharmony-build/logs/hcodec/cppcrash-hcodec_demo_arm-0-20260416152117`

关键栈：

```text
Reason: Signal:SIGSEGV(SEGV_MAPERR)
Tid: OS_IPC_2_12798
#01 libsurface.z.so(OHOS::BufferQueue::CallConsumerListener()+104)
#02 libsurface.z.so(OHOS::BufferQueue::FlushBuffer(...)+760)
#03 libsurface.z.so(OHOS::BufferQueueProducer::FlushBuffer(...)+68)
```

## 5. 结论

### 已坐实

1. 官方 `hcodec_demo.cpp` 已经按当前板子可运行的 32-bit 路线手工重编成功。
2. 新 binary `/data/local/tmp/hcodec_demo_arm` 本身可以在板子上运行。
3. 官方 sample 的 `apiType=1` buffer mode 在当前板子上可以正常工作，并跑到 EOS。
4. 官方 sample 的 `apiType=1` surface mode 在当前板子上会崩溃。
5. 这个崩溃点和自定义 `hcodec_minidec` 修正调用顺序后的 surface crash 高度一致，都落在：
   - `libsurface.z.so -> OHOS::BufferQueue::CallConsumerListener()`

### 这意味着

> 当前 surface 问题已经不是 `hcodec_minidec` 私有实现的问题了。哪怕换成官方 sample，只要真的走到 surface 输出并触发 consumer listener，当前板子的 `libsurface` / 图形栈 / 媒体栈组合也会在同一类路径上崩溃。

## 6. 一句话总结

> 官方 `hcodec_demo` 已经被成功手工重编成当前板子可运行的 32-bit 版本，并证明了一个关键结论：官方 `apiType=1` buffer 路能正常跑通，但官方 `surface` 路也会在 `libsurface.z.so -> BufferQueue::CallConsumerListener()` 处崩溃，所以 surface 问题不是 `hcodec_minidec` 独有，而是当前板端 surface callback/dispatch 路径的系统级问题。
