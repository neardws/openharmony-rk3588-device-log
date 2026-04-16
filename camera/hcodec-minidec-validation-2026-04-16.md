# hcodec_minidec 实测记录（2026-04-16）

> 目标：在 RK3588 / OpenHarmony 目标板上，用当前 `hcodec_minidec` 的 **32-bit ARM 版本** 做一轮真正的 surface / buffer 对照验证，避免只停留在源码推断。

## 1. 本轮前提

### 1.1 设备连接

本轮实际执行链路已经恢复：

```text
Ubuntu -> Windows relay -> hdc.exe -> OpenHarmony device
```

设备侧可确认：

- `hdc.exe list targets` 能看到目标设备
- `hdc.exe shell uname -a` 返回：

```text
Linux localhost 5.10.208 #1 SMP Fri Apr 25 17:16:06 CST 2025 aarch64
```

### 1.2 为什么不用 `/data/local/tmp/hcodec_minidec`

板子上已有的 `/data/local/tmp/hcodec_minidec` 依然是不能直接执行的那版，表现为：

```text
/bin/sh: /data/local/tmp/hcodec_minidec: No such file or directory
```

这不是文件缺失，而是此前已经确认过的 **ABI / loader mismatch**。

因此本轮继续使用能跑的 32-bit 路线，并把最新源码重新手工编成：

- `/data/local/tmp/hcodec_minidec_arm`

## 2. 本轮使用的 binary

### 2.1 手工 32-bit 构建

本地使用：

- `manual-arm-build/build_minidec_arm.sh`

把当前源码：

- `ohos-src/foundation/multimedia/av_codec/test/unittest/hcodec_test/demo/hcodec_minidec.cpp`

重新编成 32-bit ARM 版本。

这次额外修了一个构建细节：

- 脚本补上 `-fno-rtti -fno-exceptions`

否则链接阶段会报：

```text
undefined symbol: typeinfo for OHOS::RefBase
```

### 2.2 构建结果

新产物验证为：

```text
ELF 32-bit LSB pie executable, ARM, interpreter /lib/ld-musl-arm.so.1
```

并已重新下发到设备：

- `/data/local/tmp/hcodec_minidec_arm`

运行 usage 可确认它已经包含本轮新增环境变量支持：

```text
env: HCODEC_MINIDEC_FRAME_RATE=60 HCODEC_MINIDEC_PROGRESS_MS=1000 HCODEC_MINIDEC_OUTPUT=surface
     HCODEC_MINIDEC_PIXEL_FORMAT=auto|surface|nv12|nv21|i420|omit
     HCODEC_MINIDEC_DRAIN=sync|async
```

## 3. 设备侧测试脚本

本轮使用设备侧 runner：

- `/data/local/tmp/hcodec_minidec_suite.sh`

对应文档：

- `camera/hcodec-minidec-surface-buffer-suite.md`

脚本会自动跑：

1. `surface_minimal`
2. `buffer_sync`
3. `buffer_async`

并同时采集：

- `hidumper -s 3002`
- `hidumper -s 3011`
- `ps -ef`
- 每个 case 的独立日志和 `summary.txt`

## 4. Smoke test：320x240 / 10s

### 4.1 输入

- `/data/local/tmp/out_320_240_10s.h264`

### 4.2 运行

```bash
HCODEC_MINIDEC_BIN=/data/local/tmp/hcodec_minidec_arm \
HCODEC_RUN_ID=smoke2_320x240_all \
/data/local/tmp/hcodec_minidec_suite.sh \
  /data/local/tmp/out_320_240_10s.h264 \
  320 240 all
```

### 4.3 结果

```text
== surface_minimal ==
[error] SetOutputSurface failed: 63570434

== buffer_sync ==
[summary] queued=602 output=602 output_bytes=69350400 max_chunk=15486 in_cb=604 out_cb=603 elapsed_ms=2476 out_fps=243.13 drain=sync surface_cb=0 surface_released=0 success=true

== buffer_async ==
[summary] queued=602 output=602 output_bytes=69350400 max_chunk=15486 in_cb=604 out_cb=603 elapsed_ms=2471 out_fps=243.63 drain=async surface_cb=0 surface_released=0 success=true
```

### 4.4 结论

- `surface` 在最小样本上仍然失败，失败点非常明确：
  - `SetOutputSurface failed: 63570434`
- `buffer` 路稳定成功
- 在小分辨率样本上，`sync` 和 `async` 几乎没有差别：
  - `243.13 fps` vs `243.63 fps`

这说明：

- 小样本下 output drain 不是主要瓶颈
- `async drain` 逻辑本身至少没有把 buffer 路跑坏

## 5. 主验证：1080p60 / 10s

### 5.1 输入

- `/data/local/tmp/test-real-lauterbrunnen-1080p60-10s-copy.h264`

### 5.2 运行

```bash
HCODEC_MINIDEC_BIN=/data/local/tmp/hcodec_minidec_arm \
HCODEC_RUN_ID=lauter_1080p60_10s_a1 \
/data/local/tmp/hcodec_minidec_suite.sh \
  /data/local/tmp/test-real-lauterbrunnen-1080p60-10s-copy.h264 \
  1920 1080 all
```

### 5.3 结果

```text
== surface_minimal ==
[error] SetOutputSurface failed: 63570434

== buffer_sync ==
[summary] queued=601 output=600 output_bytes=1869350400 max_chunk=266733 in_cb=605 out_cb=601 elapsed_ms=30286 out_fps=19.81 drain=sync surface_cb=0 surface_released=0 success=true

== buffer_async ==
[summary] queued=601 output=600 output_bytes=1869350400 max_chunk=266733 in_cb=605 out_cb=601 elapsed_ms=29807 out_fps=20.13 drain=async surface_cb=0 surface_released=0 success=true
```

### 5.4 结论

#### surface 路

- 仍然没有进入真正的 surface 输出阶段
- 失败点依旧卡在：

```text
SetOutputSurface failed: 63570434
```

- 因为 surface 根本没绑成功，所以：
  - `surface_cb=0`
  - `surface_released=0`

这说明当前证据链非常稳定：

> 不是 surface consumer drain 没写，甚至还没走到真正出 surface buffer 的阶段，当前板子/媒体栈在 `SetOutputSurface` 这一步就拒掉了。

#### buffer 路

- `buffer_sync`: `19.81 fps`
- `buffer_async`: `20.13 fps`

也就是说：

- 新增 async drain 在 1080p60 / 10s 样本上 **只有极小提升**
- 提升约：`+0.32 fps`，量级大约 `1.6%`

这基本说明：

> 当前 `buffer` 路的主瓶颈并不只是 output drain，同步改成异步以后没有出现数量级改善。

## 6. 更贴近 IPC 主码流的验证：2688x1520 / 60fps / 10s

### 6.1 输入

- `/data/local/tmp/test-h264-2688x1520-main-60fps-10s.h264`

### 6.2 运行

这轮先只看 buffer 吞吐，不重复跑已经稳定失败的 surface case：

```bash
HCODEC_MINIDEC_BIN=/data/local/tmp/hcodec_minidec_arm \
HCODEC_RUN_ID=ipc_2688x1520_60fps_10s_b1 \
/data/local/tmp/hcodec_minidec_suite.sh \
  /data/local/tmp/test-h264-2688x1520-main-60fps-10s.h264 \
  2688 1520 buffer
```

### 6.3 结果

```text
== buffer_sync ==
[summary] queued=600 output=600 output_bytes=3677184000 max_chunk=84360 in_cb=603 out_cb=601 elapsed_ms=59040 out_fps=10.16 drain=sync surface_cb=0 surface_released=0 success=true

== buffer_async ==
[summary] queued=600 output=600 output_bytes=3677184000 max_chunk=84360 in_cb=603 out_cb=601 elapsed_ms=58000 out_fps=10.34 drain=async surface_cb=0 surface_released=0 success=true
```

### 6.4 结论

- 在更贴近 IPC 主码流的 `2688x1520 60fps` 样本上，buffer 路吞吐进一步降到大约 `10 fps`
- `async` 相比 `sync` 仍然只有非常小的提升：
  - `10.16 fps` -> `10.34 fps`
- 这比 `1080p60 / 10s` 的约 `20 fps` 更差，符合“分辨率越高，当前 test path 越跟不上”的趋势

这意味着：

> 当前 `buffer` 路不仅达不到 real-time 60 fps，而且在更接近 IPC 主码流分辨率时，吞吐会进一步掉到约 10 fps，说明瓶颈仍在 end-to-end test pipeline，而不是简单一个 drain 开关就能解决。

## 7. 当前最稳的判断

### 7.1 已被实测坐实的点

1. **当前 32-bit `hcodec_minidec_arm` 可以在目标板上稳定运行**
2. **buffer 模式可稳定解码并到 EOS**
3. **surface 模式在 `SetOutputSurface` 阶段失败**
4. **失败码稳定复现为 `63570434`**
5. **1080p60 / 10s buffer 路吞吐大约只有 20 fps，明显未达 real-time 60 fps**
6. **2688x1520 / 60fps / 10s buffer 路吞吐大约只有 10 fps，离 real-time 更远**
7. **async drain 不是当前主要性能瓶颈**

### 7.2 还没有被坐实的点

1. `63570434` / `0x3ca0202` 的精确错误码映射仍未在源码里定位到
2. 还不能证明 surface 路“只要继续改代码就一定能通”
3. 还不能证明当前 RK3588 / OpenHarmony 组合能交付 real-time surface/zero-copy 解码链路

## 8. 本轮产物

本轮 `1080p60 10s` 的完整日志已拉回本地：

- `/home/neardws/openharmony-build/logs/hcodec/lauter_1080p60_10s_a1/`

其中包括：

- `surface_minimal.log`
- `buffer_sync.log`
- `buffer_async.log`
- `summary.txt`
- `hidumper_3002_*`
- `hidumper_3011_*`
- `ps_*`

## 9. 一句话总结

> 到 2026-04-16 这轮真机验证为止，`hcodec_minidec` 的 buffer 路已经能在 RK3588 / OpenHarmony 目标板上稳定跑完，但 1080p60 真实样本吞吐只有约 20 fps，`2688x1520 60fps` 样本更是只有约 10 fps；surface 路则稳定卡在 `SetOutputSurface failed: 63570434`，说明当前问题已经从“能不能跑起来”收敛成了“surface 输出为何被媒体栈拒绝，以及 buffer 路为什么离 real-time 仍差很多”。
