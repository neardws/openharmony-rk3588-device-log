# 设备侧视频硬解能力初检（2026-04-15）

> 目标：在已经通过 `hdc` 进入新设备后，不先跑复杂业务链，而是先验证这台设备和当前 OpenHarmony 镜像是否具备视频硬解相关的系统能力。

## 1. 连接路径

本次不是在 Linux 主机本地直接调用 `hdc`，而是走下面这条链路：

```text
Ubuntu -> SSH 到 Windows 中转机 -> Windows 上的 hdc.exe -> 目标设备
```

已确认：

- Windows 中转机可达
- Windows 上 `hdc.exe` 可正常列出设备
- 当前设备 ID：`dd011a41443631413010125003c5ac00`

## 2. 设备基础信息

通过设备 shell 查询到：

- Kernel: `5.10.208`
- `const.product.name`: `OpenHarmony 3.2`
- `const.ohos.fullname`: `OpenHarmony-5.0.3.135`

说明这台设备当前运行的是 OpenHarmony 用户态系统，底层是 Linux 5.10 内核。

## 3. 系统侧媒体服务是否存在

设备上已确认存在：

- `/system/profile/av_codec_service.json`
- `/system/profile/media_service.json`

其中：

### 3.1 av_codec_service

`/system/profile/av_codec_service.json` 内容显示：

- process = `av_codec_service`
- system ability = `3011`
- libpath = `libav_codec_service.z.so`

### 3.2 media_service

`/system/profile/media_service.json` 内容显示：

- process = `media_service`
- system ability = `3002`
- libpath = `libmedia_service.z.so`

这说明：

- AVCodec 服务在系统里已注册
- 媒体总服务也在系统里已注册

## 4. 运行态服务检查

通过 `hidumper` 实际检查：

### 4.1 `hidumper -s 3011`

返回中可见：

- `AVCodecService`
- `Codec_Server`

这说明：

- `av_codec_service` 不只是 profile 文件存在，而是运行态可访问

### 4.2 `hidumper -s 3002`

返回中可见：

- `PlayerDistributedService`
- `PlayerServer`
- `RecorderServer`
- `CodecServer`
- `AVMetaServer`
- `TranscoderServer`
- `ScreenCaptureServer`

这说明：

- `media_service` 运行态也正常
- 媒体子服务里已经包含 codec 相关服务对象

## 5. 系统 codec 库是否存在

设备上已确认存在：

- `/system/lib/libhcodec.z.so`
- `/system/lib/libfcodec.z.so`

这说明：

- hcodec / fcodec 相关系统库已进镜像
- 系统侧并不是“没有 codec 库的空壳”

## 6. 系统公开 codec 能力检查

设备上的 `/system/etc/codec/codec_caps.xml` 已拉取并查看。

其中明确可见：

- `codecName="avdec_h264"`
- `codecType="VIDEO_DECODER"`
- `mimeType="video/avc"`
- `width="2-3840"`
- `height="2-2160"`
- `format="NV12,NV21"`

这至少可以明确说明：

> 当前系统公开声明层中，H.264 / AVC 视频解码能力是明确存在的。

## 7. vendor HDF codec 能力检查

设备上确认存在：

- `/vendor/etc/hdfconfig/codec_adapter_capabilities.hcb`

已通过 `hdc file recv` 拉回并做 ASCII 提取，结果里明确出现：

- `VideoHwEncoders`
- `HDF_video_hw_enc_avc_rk`
- `rk.video_encoder.avc`
- `VideoHwDecoders`
- `HDF_video_hw_dec_avc_rk`
- `rk.video_decoder.avc`
- `HDF_video_hw_dec_hevc_rk`
- `rk.video_decoder.hevc`

这说明：

- vendor HDF codec adapter 层里存在 Rockchip 硬编解码能力声明
- AVC/H.264 硬解和 HEVC/H.265 硬解线索都已经出现

## 8. 当前可以下的结论

基于本次设备侧检查，当前可以比较稳地说：

### 可以明确确认的

- 设备运行的是 OpenHarmony + Linux 5.10 组合
- `av_codec_service` 存在且运行态可访问
- `media_service` 存在且运行态可访问
- 系统公开 codec 能力里明确暴露了 `video/avc` / `avdec_h264`
- 系统 codec 库 `libhcodec.z.so` / `libfcodec.z.so` 已在设备中
- vendor HDF codec capability 中存在 Rockchip 硬编解码声明

### 还不能只靠这一轮就 100% 证明的

- 实际业务解码时一定走的是硬件路径，而不是软解 fallback
- H.265 / HEVC 在当前系统公开声明层已经完全打通
- RTSP 输入场景下长时间稳定性一定没问题

## 9. 本轮验证的定位

这次验证属于：

> **硬件能力存在性验证**

而不是：

> **完整硬解链路跑通验证**

也就是说，现在已经能确认“系统里有这套东西”，但下一步仍然需要通过实际解码样本来确认：

- 能不能成功创建 decoder
- 能不能稳定出帧
- CPU 占用是否显著低于软解
- 是否真的走 Rockchip 硬件路径

## 10. 下一步建议

建议按这个顺序推进：

1. 先上本地 H.264 裸流样本
2. 先做最小解码验证
3. 记录 CPU / 内存 / 是否出帧
4. 再做软解和硬解对照测试
5. 最后再接 RTSP 真流场景

## 11. `hcodec_minidec` 最小解码 sample 的补充验证

在完成前面的“系统能力存在性检查”后，继续做了一个更小、更可控的真实解码验证路径，目标不是播放器链路，而是只验证：

- `video/avc` decoder 能否被成功创建
- 能否完成 `Configure -> Start`
- 能否吃到 Annex-B H.264 输入
- 能否真正收到 output callback
- 能否正常走到 EOS

### 11.1 sample 设计

新增最小 sample：

- 源码：`/home/neardws/openharmony-build/ohos-src/foundation/multimedia/av_codec/test/unittest/hcodec_test/demo/hcodec_minidec.cpp`

设计约束：

- 不依赖 `surface`
- 不碰现成 helper
- 直接走 `VideoDecoderFactory::CreateByMime("video/avc")`
- 直接解析 Annex-B H.264 裸流，而不是沿用现成样例里的“4 字节长度前缀 + NAL”喂数格式
- 通过 callback 接 `OnInputBufferAvailable` / `OnOutputBufferAvailable`
- 支持可选输出 YUV 文件

命令行格式：

```text
hcodec_minidec <input.h264> <width> <height> [output.yuv]
```

### 11.2 构建接入

已在：

- `foundation/multimedia/av_codec/test/unittest/hcodec_test/demo/BUILD.gn`

新增：

- `ohos_executable("hcodec_minidec")`

当前构建配置要点：

- `sources = [ "hcodec_minidec.cpp" ]`
- `deps = [ "$av_codec_root_dir/interfaces/inner_api/native:av_codec_client" ]`
- `external_deps` 至少包含：
  - `ipc:ipc_single`
  - `media_foundation:media_foundation`
  - `media_foundation:native_media_core`

同时，为了让该测试 target 通过当前产品构建的 sanitizer 检查，在：

- `vendor/hihope/rk3568/security_config/sanitizer_check_list.gni`

的 `bypass_av_codec` 中追加了：

- `"hcodec_minidec"`

### 11.3 编译过程中实际遇到的问题

这次不是一次编过，中间真实踩到了几类问题：

1. **宿主机构建环境问题**
   - 最早先遇到 Python 运行时异常，典型报错包括：
     - `ModuleNotFoundError: No module named 'encodings'`
     - `ModuleNotFoundError: No module named 're'`
   - 后续改用 `prebuilts/python_llvm/linux-x86/3.11.4` 作为 `PYTHONHOME/PYTHONPATH`，把这类前缀问题绕开。

2. **`av_codec` target 的 CFI 校验**
   - GN 生成阶段先被 sanitizer 规则拦住。
   - 处理方式不是改 sample 逻辑，而是把 `hcodec_minidec` 加入当前产品的 `bypass_av_codec` 白名单。

3. **跨组件头目录引用违规**
   - 曾直接把 `//foundation/multimedia/media_foundation/interface/inner_api/` 塞进 `include_dirs`。
   - GN 明确报错：
     - `Do not directly use header files of other components`
   - 后续去掉这条直接 include 目录，改为通过规范依赖解决。

4. **链接缺失**
   - 在真正到 link 阶段时，出现：
     - `undefined symbol: OHOS::MediaAVCodec::VideoDecoderFactory::CreateByMime(...)`
   - 这是因为 sample 只 include 了头，但没有把 `av_codec_client` 真正链接进来。
   - 补上：
     - `deps = [ "$av_codec_root_dir/interfaces/inner_api/native:av_codec_client" ]`
   - 之后重新编译成功。

### 11.4 编译产物

最终 `hcodec_minidec` 已成功编过，产物为：

- stripped：`/home/neardws/openharmony-build/ohos-src/out/rk3568/multimedia/av_codec/hcodec_minidec`
- unstripped：`/home/neardws/openharmony-build/ohos-src/out/rk3568/exe.unstripped/multimedia/av_codec/hcodec_minidec`

本地检查结果：

- `ELF 32-bit LSB pie executable, ARM`
- 动态依赖最少包括：
  - `libav_codec_client.z.so`
  - `libmedia_foundation.z.so`
  - `libsec_shared.z.so`
  - `libc.so`
  - `libc++.so`

这与此前对设备的判断一致：

- `uname -m = aarch64`
- `getconf LONG_BIT = 32`

即当前设备是 **aarch64 内核 + 32-bit 用户态**，所以 `rk3568` 下生成的 32-bit ARM 可执行文件是正确方向。

## 12. 下发与真机验证

### 12.1 下发策略

仍沿用：

```text
Ubuntu -> SSH 到 Windows 中转机 -> Windows 上 hdc.exe -> 目标设备
```

继续坚持“不污染系统分区”的原则，把 sample 和补充库全部下发到：

- `/data/local/tmp`
- `/data/local/tmp/hcodec_lib`

本次实际下发的核心文件包括：

- `/data/local/tmp/hcodec_minidec`
- `/data/local/tmp/hcodec_lib/libav_codec_client.z.so`
- `/data/local/tmp/hcodec_lib/libmedia_foundation.z.so`
- `/data/local/tmp/hcodec_lib/libcodec_proxy_4.0.z.so`

设备上此前已经存在或已补齐的本地辅助库包括：

- `/data/local/tmp/hcodec_lib/libhilog.so`
- `/data/local/tmp/hcodec_lib/libsec_shared.z.so`
- `/data/local/tmp/hcodec_lib/libsurface.z.so`
- `/data/local/tmp/hcodec_lib/libutils.z.so`

同时在设备系统里继续确认到一批可直接复用的库，不必重复下发，例如：

- `/system/lib/platformsdk/libav_codec_client.z.so`
- `/system/lib/platformsdk/libmedia_foundation.z.so`
- `/system/lib/chipset-pub-sdk/libipc_single.z.so`
- `/system/lib/chipset-pub-sdk/libhilog.so`
- `/system/lib/chipset-pub-sdk/libsec_shared.z.so`
- `/system/lib/chipset-pub-sdk/libsurface.z.so`
- `/system/lib/chipset-pub-sdk/libutils.z.so`
- `/system/lib/chipset-pub-sdk/libsync_fence.z.so`
- `/system/lib/chipset-pub-sdk/libsamgr_proxy.z.so`
- `/system/lib/chipset-pub-sdk/libbegetutil.z.so`
- `/system/lib/chipset-pub-sdk/libhitrace_meter.so`
- `/system/lib/libohosffmpeg.z.so`
- `/system/lib/libqos.z.so`
- `/system/lib/libav_codec_media_engine_modules.z.so`
- `/system/lib/libav_codec_service_dfx.z.so`
- `/system/lib/libav_codec_service_utils.z.so`
- `/system/lib/platformsdk/libressched_client.z.so`
- `/system/lib/chipset-pub-sdk/libclang_rt.ubsan_minimal.so`

### 12.2 真机运行命令

输入样本继续使用：

- `/data/local/tmp/out_320_240_10s.h264`

实际运行时使用的库路径为：

```sh
export LD_LIBRARY_PATH=/system/lib:/system/lib/platformsdk:/system/lib/chipset-pub-sdk:/data/local/tmp/hcodec_lib
```

执行命令：

```sh
/data/local/tmp/hcodec_minidec \
  /data/local/tmp/out_320_240_10s.h264 \
  320 240 \
  /data/local/tmp/hcodec_minidec_320x240.yuv
```

### 12.3 真机运行结果

本次运行不是“能启动一下”就结束，而是完整跑到了 output 和 EOS。

运行日志中可持续看到：

- 多次 `[input] idx=... size=... ret=0`
- 多次 `[output] idx=... size=115200 flag=0x0 total_out=...`
- 最终出现 EOS：
  - `[output] idx=19 size=0 flag=0x1`

最终 summary 为：

```text
[summary] queued=611 output=602 output_bytes=69350400 success=true
```

这说明最小闭环已经真实成立：

1. `CreateByMime("video/avc")` 成功
2. decoder `Configure/Start` 成功
3. Annex-B H.264 裸流成功喂入
4. decoder 持续产出 output callback
5. 正常收到 EOS 并完成收尾

## 13. 本轮验证后的结论升级

到这一步，结论已经比“系统能力存在”更强一层。

之前只能确认：

- 系统里有 AVCodec 服务
- 系统里有 codec 库
- 系统公开能力里有 `video/avc`
- vendor HDF 里有 Rockchip 硬解声明

现在已经能进一步确认：

> 在这台设备上，基于 OpenHarmony AVCodec/HCodec 的 `video/avc` 解码最小链路已经被真实跑通，不是只停留在 profile / xml / capability 声明层。

换句话说：

- 这台设备的 H.264 解码能力不是“纸面支持”
- 而是至少已经在一个最小、可控、无 surface 的 inner API sample 上完成了真机输入、出帧和 EOS 验证

## 14. 一句话总结

> 这台设备当前已经可以确认具备视频硬解相关的系统服务、codec 库、公开 H.264 解码能力以及 Rockchip vendor 硬编解码声明，并且通过 `hcodec_minidec` 完成了从编译、下发到真机样本解码成功的最小闭环验证，`video/avc` 解码链路已被实际跑通。
