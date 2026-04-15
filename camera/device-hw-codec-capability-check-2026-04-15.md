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

## 11. 一句话总结

> 这台设备当前已经可以确认具备视频硬解相关的系统服务、codec 库、公开 H.264 解码能力以及 Rockchip vendor 硬编解码声明，下一步应进入实际样本解码验证，而不是继续停留在“能力是否存在”的讨论阶段。
