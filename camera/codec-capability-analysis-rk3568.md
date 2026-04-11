# RK3568 OpenHarmony 编解码能力声明层梳理

> 分析对象：本次成功构建的 `rk3568@hihope` OpenHarmony 镜像最终产物。目标是先回答一个问题：**系统声明层到底有没有暴露出后续摄像头 RTSP 硬解验证所需的能力。**

## 1. 分析范围

本次主要查看了以下产物文件：

- `out/rk3568/packages/phone/system/etc/codec/codec_caps.xml`
- `out/rk3568/packages/phone/system/profile/av_codec_service.json`
- `out/rk3568/packages/phone/system/profile/media_service.json`
- `out/rk3568/packages/phone/vendor/etc/hdfconfig/codec_adapter_capabilities.hcb`
- `out/rk3568/packages/phone/vendor/lib/...`
- `out/rk3568/packages/phone/vendor/lib/passthrough/indirect/...`

## 2. 已确认存在的服务层

### 2.1 av_codec_service

系统产物中存在：

- `system/profile/av_codec_service.json`

并且其进程名是：

- `av_codec_service`

同时 system ability 里存在：

- `name = 3011`
- `libpath = libav_codec_service.z.so`

这说明：

- OpenHarmony 的 AVCodec 服务在镜像里是被注册的
- `hcodec_demo` / AVCodec 相关能力至少在服务层是存在入口的

### 2.2 media_service

系统产物中存在：

- `system/profile/media_service.json`

并且其进程名是：

- `media_service`

同时 system ability 里存在：

- `name = 3002`
- `libpath = libmedia_service.z.so`

这说明：

- 基础媒体服务也在镜像中
- 后续摄像头流、demux、媒体框架联动至少具备系统服务入口

## 3. system codec_caps.xml 里看到的内容

`system/etc/codec/codec_caps.xml` 中可以看到标准 AVCodec 能力声明。

在本次产物里，解析到的 `VideoCodecs` 条目数量是：

- `COUNT = 5`

至少确认到：

- `avdec_h264 | VIDEO_DECODER | video/avc`
- `avdec_h263 | VIDEO_DECODER | video/h263`

这说明：

- 系统层明确声明了 **H.264 视频解码能力**
- 至少从 API 暴露角度，`video/avc` 是存在 decoder 条目的

## 4. 一个非常重要的现象

从当前产物看，`codec_caps.xml` 呈现出来的内容，和 vendor/HDF 层看到的 Rockchip 硬件能力，**不完全是同一视角**。

也就是说：

- `codec_caps.xml` 更像是系统媒体框架侧的公开能力声明
- `vendor/etc/hdfconfig/codec_adapter_capabilities.hcb` 更像是 HDF / codec adapter / 底层硬件适配层声明

这两个层次之间，不能简单画等号。

## 5. vendor HDF 能力里看到的硬件线索

在：

- `vendor/etc/hdfconfig/codec_adapter_capabilities.hcb`

做字符串提取后，已经能明确看到这些关键字：

- `VideoHwEncoders`
- `HDF_video_hw_enc_avc_rk`
- `rk.video_encoder.avc`
- `VideoHwDecoders`
- `HDF_video_hw_dec_av...`

虽然当前只通过字符串方式做了轻量提取，但已经足够说明：

> **vendor HDF codec adapter 层里，确实存在 Rockchip 硬件视频编解码相关能力声明。**

至少 AVC/H.264 方向已经看到明确线索。

## 6. vendor 库层已经确认的关键媒体库

在最终包目录中已经确认存在：

- `vendor/lib/libcodec_driver.z.so`
- `vendor/lib/libcodec_oem_interface.z.so`
- `vendor/lib/libcodec_stub_4.0.z.so`
- `vendor/lib/passthrough/libcodec_component_manager_service_4.0.z.so`

以及更关键的 Rockchip / OMX / MPP 库：

- `vendor/lib/passthrough/indirect/libOMX_Core.z.so`
- `vendor/lib/passthrough/indirect/libOMX_Pluginhw.z.so`
- `vendor/lib/passthrough/indirect/libomxvpu_dec.z.so`
- `vendor/lib/passthrough/indirect/libomxvpu_enc.z.so`
- `vendor/lib/passthrough/indirect/librockchip_mpp.z.so`
- `vendor/lib/passthrough/indirect/librockchip_vpu.z.so`
- `vendor/lib/passthrough/indirect/librga.z.so`

这说明：

- 底层硬件编解码用户态库已经进包
- OMX / MPP / codec adapter 相关实现已经进入最终镜像内容

## 7. 目前最合理的技术判断

基于现有结果，更稳妥的判断是：

### 可以明确说的

- OpenHarmony 的 `av_codec_service` 已注册
- `media_service` 已注册
- 系统层明确有 `video/avc` 解码能力声明
- vendor 层明确已有 Rockchip codec / OMX / MPP 相关库
- HDF codec adapter 能力里已经能看到 Rockchip 硬编解码线索

### 还不能 100% 直接说死的

- `codec_caps.xml` 是否已经完整反映了所有 vendor 硬件能力
- H.265/HEVC 的系统公开能力在当前产物里是否已经完全打通
- `hcodec_demo` 最终是否一定会落到 Rockchip 硬件路径，而不是某种中间 fallback

## 8. 为什么这仍然是积极信号

因为当前已经不是“只有几个库躺在 vendor 目录里”。

现在同时具备了：

1. 系统服务层：
   - `av_codec_service`
   - `media_service`
2. 系统能力声明层：
   - `codec_caps.xml`
3. vendor 硬件适配层：
   - `codec_adapter_capabilities.hcb`
4. vendor 实现库层：
   - `libOMX_Pluginhw.z.so`
   - `libomxvpu_dec.z.so`
   - `librockchip_mpp.z.so`
   - 等等

这说明整条链已经不只是“单点集成”，而是有比较完整的多层支撑。

## 9. 对后续验证工作的意义

这份分析对下一步最重要的意义是：

### 9.1 `hcodec_demo` 值得继续推进

因为系统服务和 codec 能力层都在，`hcodec_demo` 并不是在空中楼阁上跑。

### 9.2 未来要特别关注“声明层 vs 实际路径”

后续实验不能只看：

- 有没有 `video/avc`
- 有没有 `libomxvpu_dec.z.so`

而必须进一步确认：

- 调用时到底走了哪条路径
- 是否真的进入 Rockchip 硬件解码器

### 9.3 H.264 比 H.265 更适合先验证

从当前看到的公开线索看，**H.264 / AVC 是最明确的一条**。

因此后续第一条验证路径建议继续优先：

- H.264
- buffer mode
- 本地裸流
- `hcodec_demo`

## 10. 当前结论

一句话总结：

> 当前镜像在“系统服务层 + codec 声明层 + vendor HDF 层 + Rockchip OMX/MPP 实现层”四个层次上，都已经能看到视频硬解相关能力的存在，其中 H.264/AVC 是目前最明确、最适合率先验证的方向。
