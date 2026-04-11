# 摄像头 RTSP 软解到硬解迁移方案

> 目标：把当前已经跑通的“IPC 摄像头 RTSP + FFmpeg CPU 软解”链路，迁移为“IPC 摄像头 RTSP + Rockchip / OpenHarmony 硬件解码”，用于后续实时处理、显示或 NPU 推理前处理。

## 1. 当前已验证的起点

当前仓库里已经确认可用：

- 摄像头网络连通
- RTSP 拉流可用
- H.264 主码流参数已知
- FFmpeg 静态二进制可在板端运行
- CPU 软解可稳定输出到 `null` / JPEG / RGB

对应文档：

- `camera/README.md`
- `camera/api.md`

当前软解链路可以抽象为：

```text
IPC 摄像头 RTSP -> FFmpeg demux -> software decoder (h264) -> RGB/YUV/JPEG
```

迁移后的目标链路：

```text
IPC 摄像头 RTSP -> demux / 取出压缩码流 -> hardware decoder -> YUV/RGB -> 后处理 / 推理
```

## 2. 为什么这里适合验证“硬解”

这里接入的是网络 IPC 摄像头，输出 RTSP H.264 码流。

这类输入天然包含“视频解码”步骤，因此非常适合验证硬件解码器是否工作。

要区分两种场景：

### 场景 A：RTSP / H.264 / H.265 网络码流

- 这是标准“视频解码”问题
- 最适合验证 MPP / OMX / AVCodec 硬解链路

### 场景 B：本地 CSI / USB 摄像头 raw 帧

- 如果输出 raw YUV，则主要是采集 / ISP / buffer 流转
- 如果输出 MJPEG / H.264，则才涉及解码

因此，当前这个 IPC RTSP 输入源，是最适合做第一阶段硬解验证的对象。

## 3. 当前镜像里已经具备的硬解基础

本次 OpenHarmony 镜像构建结果里，已经确认这些库进入最终产物：

- `librockchip_mpp.z.so`
- `librockchip_vpu.z.so`
- `libomxvpu_dec.z.so`
- `libomxvpu_enc.z.so`
- `libOMX_Pluginhw.z.so`
- `librga.z.so`

同时系统中还确认存在：

- `system/profile/av_codec_service.json`
- `system/etc/codec/codec_caps.xml`
- `system/bin/hcodec_demo`
- `system/etc/init/media_service.cfg`

这说明：

- Rockchip 用户态编解码库已经打进镜像
- OpenHarmony 媒体服务 / codec service 也在镜像里
- 当前最缺的是“运行时验证”和“最小验证程序/路径”

## 4. 迁移时保持不变的部分

为减少变量，建议迁移时先固定以下内容：

- 同一个 IPC 摄像头
- 同一个 RTSP URL
- 同一个网络连接方式
- 同样先用 H.264 主码流验证

也就是说，先不要同时切换：

- 摄像头型号
- 输入协议
- 码流格式
- 上层业务逻辑

先只替换“decoder”这一层。

## 5. 三条可选迁移路线

## 路线 1：保留 FFmpeg，只替换为硬件解码后端

目标：

```text
RTSP -> FFmpeg demux -> rkmpp / OMX hw decoder -> frame output
```

### 优点

- 对现有 `camera/README.md` 改动最小
- 最容易与现有软解命令做 A/B 对照
- 最适合先验证“硬解是否真的被调用”

### 风险

- OpenHarmony 上编译带 `--enable-rkmpp` 或合适 OMX 后端的 FFmpeg 可能不顺
- 即使二进制能运行，也要小心是否 silently fallback 到软解

### 适用时机

- 第一优先尝试
- 适合快速出第一轮结果

## 路线 2：保留 RTSP 输入，直接调用 Rockchip MPP C API

目标：

```text
RTSP -> 取出 H.264 NAL -> MPP decoder -> YUV frame
```

### 优点

- 最接近 Rockchip 原生硬解路径
- 不依赖 FFmpeg 的硬件后端适配质量
- 最适合后续接 RGA、共享内存、NPU 前处理

### 风险

- 需要自己处理 bitstream feeding、packet/frame、buffer 生命周期
- 开发工作量高于路线 1

### 适用时机

- 路线 1 卡住时，直接转这条
- 想要更可控、更底层的验证时

## 路线 3：走 OpenHarmony 原生 AVCodec / codec service

目标：

```text
RTSP -> demux -> OHOS codec service / AVCodec -> frame output
```

### 优点

- 更贴近 OpenHarmony 官方媒体框架
- 后续更适合系统级集成

### 风险

- 文档、样例、可观测性未必比 MPP 好
- 对“快速证明 Rockchip 硬解可用”未必最快

### 适用时机

- 第二阶段
- 在硬解已经初步验证后，再做原生集成

## 6. 推荐迁移顺序

建议按下面的顺序推进：

### 第一步：保留 RTSP 摄像头输入不变

继续用已经验证过的：

- `rtsp://admin:a1234567~@192.168.1.13:554/media/video1`

### 第二步：把“拉流”和“解码”两个层次拆开

原先一个 FFmpeg 命令把所有事都做了。

迁移时应拆成两个概念：

1. 输入层
   - RTSP 拉流
   - H.264 包提取 / Annex-B 码流获取

2. 解码层
   - software decoder
   - hardware decoder

这样才能在输入不变的前提下替换 decoder。

### 第三步：先做到“硬解出帧/丢帧”，不要先追求显示

首个成功目标建议是：

- 能吃 RTSP 的 H.264 码流
- 能经硬件解码输出 YUV frame 或直接写 `/dev/null` 型 sink
- 能证明 CPU 明显低于软解

先别把目标设成：

- 完整播放器
- UI 显示
- 图像叠加
- 推理联动

### 第四步：硬解稳定后，再接后处理链

后续再逐步接：

- JPEG 导出
- RGA 转 RGB
- NPU 推理
- 屏幕显示 / GUI 预览

## 7. 推荐的技术路线判断

如果目标是“最快证明硬解已经工作”，推荐顺序是：

1. **FFmpeg + 硬解后端尝试**
2. **直接 MPP C API 最小解码程序**
3. **OHOS 原生 codec service / hcodec_demo 路线**

原因：

- 路线 1 改动最小
- 路线 2 最稳最可控
- 路线 3 最原生，但不一定最快得到可判定结论

## 8. 验证时不能只看“有没有出图”

必须同时看以下指标：

### 8.1 CPU 占用

硬解最直接的价值就是降低 CPU。

如果从软解切换到“硬解”后 CPU 没明显变化，要高度怀疑：

- 仍在走软解 fallback
- 只是 demux 变了，decoder 没变

### 8.2 帧率 / 实时性

需要至少对比：

- 子码流
- 主码流
- 持续 30 秒以上

关注：

- 是否掉帧
- 是否卡顿
- `speed` 是否持续 >= 1.0x

### 8.3 日志痕迹

需要记录媒体相关日志，重点关注：

- `media_service`
- `av_codec_service`
- `codec`
- `omx`
- `mpp`
- `rockchip`

### 8.4 长时间稳定性

至少跑一次 5~10 分钟压力验证，观察：

- 内存是否持续上涨
- 是否出现卡死 / 崩溃 / 花屏
- 是否出现媒体服务重启

## 9. 建议的里程碑

### 里程碑 A

继续保持当前软解能力稳定：

- RTSP 拉流
- H.264 主码流
- 能持续解码

### 里程碑 B

首个硬解验证成功：

- RTSP 输入不变
- 硬件 decoder 跑通
- 先输出到 YUV 或 null sink

### 里程碑 C

加入最小后处理：

- 单帧导出
- RGA 转换
- 保存 JPEG / RGB

### 里程碑 D

进入业务链路：

- 接 NPU 推理
- 接显示
- 接实时处理管线

## 10. 建议先做什么

对当前项目，最合理的下一步是：

1. 保留现有 IPC RTSP 输入
2. 做一个“上板硬解验证 checklist”
3. 优先尝试 FFmpeg 硬解后端
4. 如果卡住，立即切 MPP C API 最小程序

## 11. 当前结论

这条迁移路径不是从零开始。

当前项目已经有：

- 可靠输入源
- 已验证软解路径
- 已确认镜像内的硬解相关库

所以现在差的不是“方向”，而是：

- 最小硬解验证实现
- 明确的板上验证标准
- 一套能复现实验结果的 checklist
