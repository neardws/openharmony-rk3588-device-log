# Ubuntu 系统信息采集（2026-03-29）

> 通过 SSH 连接采集，板子当前运行官方 Ubuntu 镜像。

## 连接方式

- SSH: `ztl@192.168.31.175` (password: 123456)
- 网口：eth1（第二个 RJ45 口）

## 系统概览

| 项目 | 值 |
|------|------|
| 系统 | Ubuntu 22.04.4 LTS (Jammy) |
| 内核 | 5.10.198 #210 SMP (2025-02-25) |
| 架构 | aarch64 |
| 主机名 | 3588 |
| 板型标识 | `ztl, A588 Firefly roc-rk3399-pc PLUS` |
| 用户 | ztl |

## 硬件配置

### CPU

- 8 核 64 位
- 4x Cortex-A76 + 4x Cortex-A55
- BogoMIPS: 48.00

### 内存

```
               total        used        free      shared  buff/cache   available
内存：      7.7Gi       1.3Gi       5.1Gi       168Mi       1.3Gi       6.2Gi
交换：         0B          0B          0B
```

### 存储（eMMC 58.3GB）

```
NAME         MAJ:MIN RM  SIZE RO TYPE MOUNTPOINTS
mmcblk0      179:0    0 58.3G  0 disk 
├─mmcblk0p1  179:1    0    4M  0 part 
├─mmcblk0p2  179:2    0    4M  0 part 
├─mmcblk0p3  179:3    0    2G  0 part 
├─mmcblk0p4  179:4    0  128M  0 part 
├─mmcblk0p5  179:5    0   32M  0 part 
├─mmcblk0p6  179:6    0   64M  0 part 
├─mmcblk0p7  179:7    0  128M  0 part 
└─mmcblk0p8  179:8    55.9G  0 part /
mmcblk0boot0 179:32   0    4M  1 disk 
mmcblk0boot1 179:64   0    4M  1 disk 
```

磁盘使用：7.2G / 56G (14%)

### 温度

~43°C（thermal_zone0: 43461，即 ~43.5°C）

## NPU / RKNPU 状态

### 驱动

- DTS 节点：`npu@fdab0000`，status = `okay`
- compatible：`rockchip,rk3588-rknpu`
- 驱动版本：`rknpu 0.9.3`（20231121）
- 使用 iommu 模式
- dmesg 关键输出：

```
[    2.645140] RKNPU fdab0000.npu: RKNPU: rknpu iommu is enabled, using iommu mode
[    2.647077] [drm] Initialized rknpu 0.9.3 20231121 for fdab0000.npu on minor 1
[    2.650935] RKNPU fdab0000.npu: RKNPU: bin=0
[    2.651138] RKNPU fdab0000.npu: leakage=8
[    2.658937] RKNPU fdab0000.npu: pvtm=873
[    2.676153] RKNPU fdab0000.npu: failed to find power_model node
[    2.676173] RKNPU fdab0000.npu: RKNPU: failed to initialize power model
```

### ⚠️ 用户态运行时缺失

- **未安装 librknnrt.so**
- **未安装 rknn_server**
- 没有 rknpu2 用户态组件
- 需要手动安装才能运行 RKNN 模型推理

### 安装步骤（待执行）

1. 从 https://github.com/rockchip-linux/rknpu2 获取 runtime
2. 部署到板端：
   - `runtime/RK3588/Linux/librknn_api/aarch64/librknnrt.so` → `/usr/lib/`
   - `runtime/RK3588/Linux/rknn_server/aarch64/rknn_server` → `/usr/bin/`
   - `start_rknn.sh` / `restart_rknn.sh` → `/usr/bin/`
3. 启动 rknn_server 并验证

## 显示

### GPU

- Mali-G610 MP4
- 设备节点：`/dev/mali0`

### DRM 输出状态

| 输出 | 状态 | 备注 |
|------|------|------|
| card0-DSI-1 | connected | MIPI DSI |
| card0-HDMI-A-1 | disconnected | 第一个 HDMI |
| card0-HDMI-A-2 | **connected** | **第二个 HDMI，当前使用** |
| card0-Writeback-1 | unknown | |

HDMI-A-2 支持分辨率：3840x2160（4K）

## 网络

| 接口 | 状态 | IP | 备注 |
|------|------|------|------|
| eth0 | DOWN | — | 第一个 RJ45 口（未接线） |
| eth1 | UP | 192.168.31.175/24 | 第二个 RJ45 口，有 IPv6 |

- 网口映射：eth1 对应物理 `网口1`（需串口日志进一步确认）

## 视频 / 摄像头

- `/dev/video0`：可用（摄像头接口）
- `/dev/video-dec0`：视频解码设备
- `/dev/video-enc0`：视频编码设备

## 分区表分析

| 分区 | 大小 | 推测用途 |
|------|------|----------|
| mmcblk0p1 | 4M | U-Boot SPL |
| mmcblk0p2 | 4M | U-Boot |
| mmcblk0p3 | 2G | recovery / backup? |
| mmcblk0p4 | 128M | boot / kernel |
| mmcblk0p5 | 32M | resource / logo |
| mmcblk0p6 | 64M | dtb? |
| mmcblk0p7 | 128M | backup? |
| mmcblk0p8 | 55.9G | rootfs（Ubuntu） |
| mmcblk0boot0 | 4M | boot0 |
| mmcblk0boot1 | 4M | boot1 |

## 待办事项

- [ ] 安装 rknpu2 用户态库（librknnrt.so + rknn_server）
- [ ] 验证 NPU 推理功能
- [ ] 确认分区表详细布局（需串口日志）
- [ ] 确认 eth0/eth1 与物理网口的映射关系
- [ ] 测试 YOLO 模型部署
- [ ] 备份当前 Ubuntu 镜像（刷 OpenHarmony 前建议备份）
