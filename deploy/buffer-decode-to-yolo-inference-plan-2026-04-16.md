# buffer decode -> YOLO inference 方案（2026-04-16）

> 目标：在当前 RK3588 / OpenHarmony 板子上，基于已经验证可运行的 **buffer 模式硬解**，把解码输出稳定送进 YOLO 推理链路。当前先不依赖 surface 路。

## 1. 工程决策

当前明确采用：

```text
H.264/H.265 bitstream -> hardware decode (buffer mode) -> frame preprocess -> YOLO inference
```

**暂不依赖：**

```text
... -> surface output -> display / zero-copy / render path
```

原因很直接：

1. `buffer` 路已被多轮实测证明可运行
2. `surface` 路已被多条证据证明存在板端系统级问题
3. 你的业务目标是“解码出来喂给 YOLO 推理”，不是显示/渲染

因此，当前阶段最优策略是：

> **先把 buffer decode -> YOLO inference 做通，再看后续是否值得回头优化 surface/zero-copy。**

---

## 2. 为什么这个决策是合理的

### 2.1 buffer 路已经能工作

实测：

- `hcodec_minidec_arm`
  - `1080p60 / 10s`: 约 `20 fps`
  - `2688x1520 / 60fps / 10s`: 约 `10 fps`
- 官方重编 `hcodec_demo_arm`
  - `apiType=1` + `buffer mode`：可跑到 EOS

说明：

- 硬解器本身在工作
- buffer 输出链路可被稳定使用

### 2.2 surface 路不只是“不优雅”，而是当前不可靠

已经确认：

- `hcodec_minidec_arm` surface 崩
- 官方 `hcodec_demo_arm(apiType=1)` surface 崩
- 纯 surface producer/consumer probe（绕过 codec）也崩

共同栈都落在：

```text
libsurface.z.so -> OHOS::BufferQueue::CallConsumerListener()
```

这意味着：

> surface 当前不是“稍微改一改就能先用”的状态，而是会把整个推理链路稳定性拖垮。

---

## 3. 推荐的总体架构

### 当前阶段（推荐）

```text
RTSP / file input
  -> demux / Annex-B reader
  -> buffer-mode hardware decoder
  -> NV12 / YUV frame queue
  -> preprocess (resize / color convert / normalize)
  -> YOLO inference
  -> result queue (boxes / classes / scores)
```

### 以后 NPU 可用时

```text
RTSP / file input
  -> buffer-mode hardware decoder
  -> preprocess
  -> NPU inference
  -> result queue
```

也就是说：

- **decoder 输出阶段先固定为 buffer mode**
- 后面 CPU ncnn / NPU 只是替换 inference backend
- 前后解耦，避免因为某个后端没准备好卡死整体验证

---

## 4. 当前最推荐的 decoder 基线

这里有两个可选基线：

### 选项 A：继续基于 `hcodec_minidec_arm`

优点：

- 代码小
- 可控性强
- 已经有吞吐数据
- 已经补了 `buffer/surface`、`sync/async`、日志等能力

缺点：

- 不是官方 sample

### 选项 B：基于官方重编 `hcodec_demo_arm(apiType=1, buffer)`

优点：

- 更官方
- 已证明 buffer mode 可跑通

缺点：

- helper 体系更重
- `apiType=0` 仍然不稳
- 代码复杂度更高，不如 `minidec` 好改

### 当前建议

> **建议把 `hcodec_minidec_arm` 作为 decoder integration 基线。**

理由：

- 目标不是证明“官方 sample 存在”，这一点已经证明了
- 目标是尽快把“可跑的 buffer decode 输出”接到 YOLO
- 为此，`hcodec_minidec_arm` 更容易改造成一个稳定的解码前端

官方 `hcodec_demo_arm(apiType=1)` 保留为对照样本，不作为主集成底座。

---

## 5. buffer 输出给 YOLO 时最重要的接口约定

YOLO 不关心 decoder 内部细节，它关心的是：

1. 帧什么时候到
2. 帧是什么格式
3. 帧尺寸是多少
4. 是否允许丢帧/跳帧

### 当前建议的数据约定

Decoder 输出：

- pixel format: `NV12`（优先）
- width / height: 原始解码尺寸
- frame timestamp: 保留
- ownership: decoder 回调拿到后，尽快复制/转换，再释放 output buffer

Inference 输入：

- target: `640x640`（YOLOv8n 常见输入）
- color: RGB / BGR（取决于推理实现）
- preprocess:
  - resize 或 letterbox
  - normalize
  - layout transform

### 关键原则

> **先接受一次必要的 CPU 侧颜色/尺寸转换，不要在当前阶段为了 zero-copy 把系统复杂度拉爆。**

---

## 6. 当前阶段不要追求什么

为了先把链路做通，当前阶段**不要**一上来追：

- zero-copy
- surface 输出
- GUI 显示
- RGA 零额外拷贝
- 60fps 实时完整闭环
- 一边显示一边推理一边录制

当前阶段真正要拿到的是：

> **板子能稳定做 buffer 硬解，并把帧送进 YOLO，拿到正确推理结果。**

---

## 7. 立即可执行的分阶段任务

## 阶段 1：把 decoder 输出稳定变成“可消费帧”

目标：

- 从 `hcodec_minidec_arm` 中拿到稳定 buffer 输出
- 明确输出是 `NV12`
- 在每帧输出时，交给一个简单的 `FrameConsumer` 接口

建议改造：

- 在 `DrainOutputs(...)` 里新增可插拔消费点
- 把当前“只统计/可选 dump”改成：
  - `OnDecodedFrame(ptr, size, width, height, pixel_format, pts)`

这样后面不管接：

- dump
- CPU YOLO
- NPU YOLO
- frame skip

都不用再动 decoder 主逻辑

## 阶段 2：先做最小 CPU YOLO 验证

目标：

- 用 320x240 或 1080p 测试流
- 取 decoder 输出帧
- 做最小 resize/convert
- 丢给 YOLOv8n ncnn
- 输出框结果

为什么先 CPU：

- NPU 当前还没通
- CPU ncnn 路已经有基础文档
- 先验证“buffer decode -> inference”数据通路正确

## 阶段 3：加上背压/丢帧策略

因为目前：

- decode 1080p60 只有 ~20 fps
- 2688x1520@60 只有 ~10 fps
- YOLO CPU 也不是 60 fps

所以必须明确策略：

### 推荐策略：latest-frame only

- decoder 持续产出帧
- inference 线程只取“最新一帧”
- 老帧可被覆盖

优点：

- 延迟可控
- 不会因为队列堆积越来越卡
- 更适合实时检测

而不是：

- 每帧都排队
- 导致结果越来越滞后

## 阶段 4：如果 CPU 验证通过，再替换 inference backend

后续再决定：

- 换 RKNN / NPU
- 加 RGA 转换
- 优化预处理

这时 decoder 前端不需要重写。

---

## 8. 当前最推荐的 immediate next step

如果只选一个最合适的下一步，我建议：

> **把 `hcodec_minidec_arm` 的 buffer 输出封成一个简单的 `FrameConsumer` 回调接口，然后接一个最小的 CPU YOLO consumer。**

也就是先做：

```text
hcodec_minidec_arm(buffer)
  -> OnDecodedFrame(...)
  -> resize/convert
  -> YOLOv8n ncnn
  -> print detections
```

这是现在最不绕、最贴近目标、最少受 surface 问题影响的路线。

---

## 9. 一句话总结

> 当前 RK3588 / OpenHarmony 项目的工程决策应明确切到 **buffer decode -> YOLO inference**：把 `hcodec_minidec_arm` 作为可控的 buffer 解码前端，用 CPU ncnn 先把推理链路做通，暂时不把 surface/zero-copy 当成当前阻塞项。
