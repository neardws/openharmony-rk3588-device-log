# hcodec_demo 使用说明与定位

> 目标：在没有摄像头、没有 RTSP、甚至暂时没有板子的情况下，先把 OpenHarmony 自带 `hcodec_demo` 的定位、输入要求、最小命令和未来验证作用整理清楚。

## 1. 它是什么

`hcodec_demo` 是 OpenHarmony `av_codec` 里的一个测试可执行文件。

源码位置：

- `foundation/multimedia/av_codec/test/unittest/hcodec_test/demo/hcodec_demo.cpp`

构建安装信息：

- 安装目标：`system/bin/hcodec_demo`
- BUILD 文件：`foundation/multimedia/av_codec/test/unittest/hcodec_test/demo/BUILD.gn`

它不是一个 RTSP 播放器，也不是 FFmpeg 替代品。

它的定位更接近：

> **给定一段本地 H.264/H.265 裸码流文件，验证 OpenHarmony hcodec / AVCodec 硬件编解码路径是否工作。**

## 2. 它不能直接做什么

根据源码分析，`hcodec_demo` **不能直接接 RTSP URL**。

原因：

- 输入参数是 `--in <local file>`
- 解码前会调用 `StartCodeDetector::SetSource(opt_.inputFile)`
- `StartCodeDetector` 是按 H.264/H.265 start code 解析 NALU 的

所以它期待的输入是：

- 本地文件
- H.264 / H.265 elementary stream
- 最好是 Annex-B / 带 start code 的裸流

它不适合直接处理：

- `rtsp://...`
- `mp4`
- `mkv`
- 复杂封装容器

## 3. 它适合放在验证链路的哪一步

推荐把它放在这个链路里：

```text
RTSP 摄像头 -> 抽取裸 H.264/H.265 文件 -> hcodec_demo -> 验证硬解
```

也就是说，`hcodec_demo` 更像一个“中间验证层”。

它验证的是：

- 镜像里的 hcodec / AVCodec / codec service 是否能解码
- 硬件解码器对本地 H.264/H.265 裸流是否正常

它**不直接验证**：

- RTSP 网络输入
- 摄像头接入
- 端到端直播流处理

## 4. 已确认的关键参数

从 `command_parser.cpp` 可以确认这些核心参数：

- `--in`：输入文件路径
- `--width`：宽度
- `--height`：高度
- `--protocol`：码流类型
  - `0 = H264`
  - `1 = H265`
  - `2 = H266`
- `--isEncoder`：是否编码器
  - `0 = decoder`
  - `1 = encoder`
- `--isBufferMode`：输出模式
  - `0 = surface mode`
  - `1 = buffer mode`
- `--apiType`：API 类型
  - `0 = codecbase`
  - `1 = new capi`
  - `2 = old capi`
- `--frameRate`：帧率
- `--timeout`：超时（ms）
- `--rotation`：旋转角度
- `--scaleMode`：缩放模式

## 5. 推荐最小命令模板

## 5.1 H.264 buffer mode

推荐作为第一条尝试命令：

```bash
/system/bin/hcodec_demo \
  --in /data/local/tmp/test_10s.h264 \
  --width 1280 \
  --height 720 \
  --protocol 0 \
  --isEncoder 0 \
  --isBufferMode 1 \
  --apiType 0 \
  --frameRate 30 \
  --timeout 30000
```

### 适用场景

- 最小硬解验证
- 先确认 decoder 能不能正常吃本地 H.264 裸流
- 尽量减少 surface / 显示链路干扰

## 5.2 H.264 surface mode

如果 buffer mode 跑通，再试：

```bash
/system/bin/hcodec_demo \
  --in /data/local/tmp/test_10s.h264 \
  --width 1280 \
  --height 720 \
  --protocol 0 \
  --isEncoder 0 \
  --isBufferMode 0 \
  --apiType 0 \
  --frameRate 30 \
  --timeout 30000
```

### 适用场景

- 验证 surface 输出路径
- 看 surface 模式是否比 buffer mode 更接近未来显示链路

## 5.3 H.265 模板

如果输入是 H.265 裸流：

```bash
/system/bin/hcodec_demo \
  --in /data/local/tmp/test_10s.h265 \
  --width 1920 \
  --height 1080 \
  --protocol 1 \
  --isEncoder 0 \
  --isBufferMode 1 \
  --apiType 0 \
  --frameRate 30 \
  --timeout 30000
```

## 6. 为什么推荐先用 buffer mode

推荐顺序是：

1. buffer mode
2. surface mode

理由：

- buffer mode 变量更少
- 不依赖窗口 / surface / 显示相关行为
- 更适合先判断“decoder 本身是否通了”

如果一上来就用 surface mode，失败时不容易判断问题出在：

- 解码器
- surface 创建
- 输出绑定
- 显示链

## 7. 它未来怎么接到 RTSP 摄像头验证里

等板子和摄像头到位后，建议分两步：

### 阶段 A：本地裸流验证

```text
H.264/H.265 文件 -> hcodec_demo
```

目标：

- 先证明 hcodec 硬解器本身能工作

### 阶段 B：接入 IPC 摄像头 RTSP

```text
RTSP -> 抽取裸流 -> hcodec_demo
```

目标：

- 在不改 hcodec_demo 的前提下，把 RTSP 输入桥接过来

## 8. 它对当前项目的价值

当前没有板子 / 摄像头，也依然可以围绕它推进：

- 固化命令模板
- 固化输入格式要求
- 准备离线测试样本
- 规划后续 RTSP -> 裸流转换步骤

这样等板子回来时，不需要再从源码重新摸起。

## 9. 当前最靠谱的使用结论

一句话总结：

> `hcodec_demo` 不是 RTSP 端到端工具，但非常适合作为“本地裸 H.264/H.265 -> OpenHarmony 硬解”这一步的最小验证器。
