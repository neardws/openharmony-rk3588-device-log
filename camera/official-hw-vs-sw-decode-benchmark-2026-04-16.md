# 官方硬解链路 vs 软件解码链路对比（2026-04-16）

> 目标：在当前 RK3588 / OpenHarmony 板子上，对比已经可运行的**官方硬解 buffer 链路**和**软件解码链路**，并给出吞吐、资源占用和稳定性结论。

## 1. 对比对象

### 1.1 硬解链路

使用：

- 手工重编的官方 sample：`/data/local/tmp/hcodec_demo_arm`
- 模式：`apiType=1` + `buffer mode`

这条链路已经确认：

- 可以稳定跑 buffer 输出
- 可以跑到 EOS
- 不依赖 surface

### 1.2 软解链路

使用：

- 板端静态 FFmpeg：`/data/local/tmp/ffmpeg`
- 模式：CPU software decode，输出到 `null`

命令核心形态：

```bash
/data/local/tmp/ffmpeg -benchmark -v info -stats -threads 0 -i <input.h264> -f null -
```

> 注意：这里的软解链路不是 OpenHarmony 官方 media sample，而是当前板端可直接使用的 CPU 软件解码 baseline。

---

## 2. 测试方法

### 2.1 输入样本

本轮对比使用两组离线样本：

1. `1920x1080 @ 60fps / 10s`
   - `/data/local/tmp/test-real-lauterbrunnen-1080p60-10s-copy.h264`
2. `2688x1520 @ 60fps / 10s`
   - `/data/local/tmp/test-h264-2688x1520-main-60fps-10s.h264`

### 2.2 指标

本轮记录这些可直接比较的指标：

- 吞吐（程序日志里的 fps）
- 实时倍速（相对 60fps）
- Real time（墙钟时间）
- User time / System time
- 总 CPU 时间（User + System）
- 折算平均 CPU 核占用：

```text
equivalent_cpu_cores = (user_time + system_time) / real_time
```

- Max RSS
- Context switch
- 是否跑到 EOS

### 2.3 资源统计来源

板端使用：

```bash
/bin/time -v <decode command>
```

因此资源占用数字来自同一类工具输出，便于直接横向比较。

---

## 3. 结果总表

## 3.1 1080p60 / 10s

| 指标 | 官方硬解 buffer (`hcodec_demo_arm apiType=1`) | 软解 (`ffmpeg`) |
|---|---:|---:|
| 成功到 EOS | ✅ | ✅ |
| 程序报告吞吐 | **123.53 fps** | **78 fps** |
| 实时倍速（相对 60fps） | **2.06x** | **1.30x** |
| Real time | **5.303903 s** | **7.786923 s** |
| User time | 0.626517 s | 28.678591 s |
| System time | 0.429897 s | 0.570217 s |
| 总 CPU 时间 | **1.056414 s** | **29.248808 s** |
| 折算平均 CPU 核占用 | **0.20 cores** | **3.76 cores** |
| 折算 4 核总占用 | **4.98%** | **93.9%** |
| Max RSS | 82968 KiB | 77808 KiB |
| Voluntary context switches | 3646 | 8353 |
| Involuntary context switches | 149 | 3538 |

## 3.2 2688x1520@60 / 10s

| 指标 | 官方硬解 buffer (`hcodec_demo_arm apiType=1`) | 软解 (`ffmpeg`) |
|---|---:|---:|
| 成功到 EOS | ✅ | ✅ |
| 程序报告吞吐 | **81.85 fps** | **63 fps** |
| 实时倍速（相对 60fps） | **1.36x** | **1.05x** |
| Real time | **7.901234 s** | **9.551500 s** |
| User time | 0.653908 s | 35.829502 s |
| System time | 0.600372 s | 0.760343 s |
| 总 CPU 时间 | **1.254280 s** | **36.589845 s** |
| 折算平均 CPU 核占用 | **0.16 cores** | **3.83 cores** |
| 折算 4 核总占用 | **3.97%** | **95.8%** |
| Max RSS | 114696 KiB | 127868 KiB |
| Voluntary context switches | 3657 | 6492 |
| Involuntary context switches | 65 | 4488 |

---

## 4. 可视化摘要

### 4.1 吞吐（fps，越高越好）

```text
1080p60 / 10s
HW  123.53 fps  |█████████████████████████
SW   78.00 fps  |███████████████

2688x1520@60 / 10s
HW   81.85 fps  |████████████████
SW   63.00 fps  |████████████
```

### 4.2 CPU 核占用（越低越好）

```text
1080p60 / 10s
HW   0.20 cores |█
SW   3.76 cores |████████████████████

2688x1520@60 / 10s
HW   0.16 cores |█
SW   3.83 cores |████████████████████
```

### 4.3 结论可视化

```text
吞吐：硬解 > 软解
CPU：硬解 << 软解
稳定性：两者在 buffer 模式下都可跑到 EOS
surface：当前不纳入此对比，因为板端 surface dispatch 已确认存在系统级问题
```

---

## 5. 怎么理解这些结果

## 5.1 吞吐上，官方硬解 buffer 路显著领先

- 在 `1080p60 / 10s` 上：
  - 硬解约 `123.53 fps`
  - 软解约 `78 fps`
- 在 `2688x1520@60 / 10s` 上：
  - 硬解约 `81.85 fps`
  - 软解约 `63 fps`

也就是说：

- 两者都能超过实时 60fps
- 但硬解明显留有更大余量
- 分辨率一提高，硬解优势仍然存在

## 5.2 资源占用上，差距比吞吐更夸张

这是本轮最关键的发现。

### 1080p60 / 10s

- 硬解总 CPU 时间：`1.056s`
- 软解总 CPU 时间：`29.249s`

换算后：

- 硬解平均只占 `0.20` 个 CPU 核
- 软解平均占了 `3.76` 个 CPU 核

### 2688x1520@60 / 10s

- 硬解平均约 `0.16` 个 CPU 核
- 软解平均约 `3.83` 个 CPU 核

这说明：

> 即使在软解也能勉强跑过实时的情况下，CPU 代价已经接近“吃满整机 4 核”的级别，而硬解 buffer 路几乎只是轻微 CPU 参与。

## 5.3 这对后续 YOLO 推理有什么意义

如果后面还要做：

- 预处理
- YOLO 推理
- 后处理
- 业务线程

那么软解链路虽然“能跑”，但已经把大部分 CPU 预算吃掉了。

硬解链路的真正价值不只是 fps 更高，而是：

> **它把 CPU 留给了后面的推理和业务逻辑。**

---

## 6. 与当前 `hcodec_minidec_arm` 的关系

这份对比也顺带说明了：

> 当前 `hcodec_minidec_arm` 那条“码流 -> buffer 输出”路径明显还没有逼近板子的真实 buffer-mode 硬解上限。

因为之前已经测到：

- `hcodec_minidec_arm`
  - `1080p60 / 10s`: 约 `20 fps`
  - `2688x1520@60 / 10s`: 约 `10 fps`

而官方 `hcodec_demo_arm(apiType=1, buffer)` 已经达到：

- `1080p60 / 10s`: `123.53 fps`
- `2688x1520@60 / 10s`: `81.85 fps`

所以：

> 板子和硬解 core 不是问题，当前主要性能瓶颈之一仍然在自定义 sample/pipeline 本身。

---

## 7. 当前最稳结论

一句话总结：

> 在当前 RK3588 / OpenHarmony 板子上，官方重编后的 `hcodec_demo_arm(apiType=1, buffer)` 已经证明：buffer 硬解链路不仅吞吐高于 CPU 软解，而且 CPU 占用只有软解的一个零头。对后续“buffer decode -> YOLO inference”来说，这意味着硬解并不只是更快，而是明显更省 CPU，更适合作为后续推理前端。
