# DC-A588-V04 OpenHarmony 系统信息（HDC 采集 2026-03-29）

> 通过 HDC USB 连接采集。已刷入官方 OpenHarmony 固件包。

## 连接方式

- HDC USB：OTG 口（上层 Type-A）→ Windows PC
- 设备 ID：`2c010e5344533753321002b19ba7ac00`
- HDC TCP 已配置：`persist.hdc.mode=tcp`, `persist.hdc.port=5555`
- 网络 IP：`192.168.31.42`（eth0，r8168 驱动）
- TCP 连接方式（重启后）：`hdc tconn 192.168.31.42:5555`

## 系统版本

| 项目 | 值 |
|------|------|
| 版本 | OpenHarmony-4.1.7.5 |
| 内核 | 5.10.110-geaba2291341c-dirty |
| 内核编译时间 | Tue Jul 22 15:49:51 CST 2025 |
| 架构 | aarch64 |
| 主机名 | localhost |

## 网络接口

| 接口 | 驱动 | IP | 状态 |
|------|------|-----|------|
| eth0 | r8168 (RTL8168, PCIe) | 192.168.31.42/24 | UP, RUNNING |
| eth1 | rk_gmac-dwmac (SoC GMAC) | 无 IP | UP, 未接线 |
| docker0 | bridge | 172.17.0.1/16 | UP |
| lo | - | 127.0.0.1 | UP |

- eth0 MAC: 3e:36:8d:6f:ff:bf
- eth1 MAC: 5e:67:7a:b6:68:f7（与 Ubuntu 系统下相同）

## eMMC 分区表（OpenHarmony 布局）

| 分区 | 大小 | 用途 |
|------|------|------|
| mmcblk0p1 | 4MB | uboot |
| mmcblk0p2 | 4MB | misc |
| mmcblk0p3 | 16MB | - |
| mmcblk0p4 | 96MB | - |
| mmcblk0p5 | 4MB | - |
| mmcblk0p6 | 2GB | system（loop 挂载 squashfs）|
| mmcblk0p7 | 512MB | vendor（ext4, ro）|
| mmcblk0p8 | 384MB | - |
| mmcblk0p9 | 256MB | - |
| mmcblk0p10 | 32MB | - |
| mmcblk0p11 | 4MB | - |
| mmcblk0p12 | 56.8GB | data（f2fs, rw）|

- zram0: 1GB（swap）
- loop0~6: squashfs 镜像挂载（system 等分区）

## 挂载情况

```
/dev/block/mmcblk0p7  → /vendor    (ext4, ro)
/dev/block/mmcblk0p12 → /data      (f2fs, rw)
/dev/block/mmcblk0p12 → /data/docker/var (f2fs, rw)
loop设备              → system 分区（squashfs 只读）
```

## Docker

- Docker 已安装并运行（定昌定制功能）
- docker0 桥接网络：172.17.0.1/16
- 数据目录：`/data/docker/var`
- 当前无容器运行（`docker ps` 为空）
- `/system/etc/docker` 通过 tmpfs 挂载

## NPU 状态 ⚠️

| 项目 | 状态 |
|------|------|
| DTS npu@fdab0000 status | okay |
| /dev/rknpu | **不存在** |
| 内核 RKNPU 驱动 | **未加载/未编译** |
| /vendor/lib 中 RKNN 库 | **无** |
| dmesg RKNPU 日志 | **无** |

**结论**：该固件内核未集成 RKNPU 驱动（rknpu.ko 或内置），`/dev/rknpu` 设备节点不存在，NPU 无法使用。DTS 中虽然 status=okay，但驱动缺失。

**对比**：Ubuntu 5.10.198 固件中 RKNPU 驱动 0.9.3 已加载，/dev/rknpu 存在。

## SELinux 状态

- 运行中（permissive 模式部分策略）
- 部分 AVC denied 日志（codec_host, av_codec_service 读取 dma_heap 被拒）

## 待确认项

```powershell
# 查内核是否支持 RKNPU（config）
.\hdc.exe shell "zcat /proc/config.gz 2>/dev/null | grep -i rknpu"

# 查 libc 类型（musl vs glibc）
.\hdc.exe shell "ls /system/lib64/libc* 2>/dev/null | head -10"

# 查是否有 RKNN 库在其他路径
.\hdc.exe shell "find /system /vendor -name 'librknn*' 2>/dev/null"
```

## 与 Ubuntu 系统对比

| 项目 | Ubuntu 5.10.198 | OpenHarmony 5.10.110 |
|------|-----------------|----------------------|
| RKNPU 驱动 | ✅ 0.9.3 已加载 | ❌ 未加载 |
| /dev/rknpu | ✅ 存在 | ❌ 不存在 |
| /dev/mali0 | ✅ | 待确认 |
| 网络 eth0 | RTL8211F (SoC) | r8168 (PCIe) ← 接口反了！|
| Docker | ❌ | ✅ 已集成 |
| SSH | ✅ 22端口开放 | 未知 |

> 注意：Ubuntu 下 eth0 是 SoC GMAC（rk_gmac），eth1 是 PCIe RTL8168。
> OpenHarmony 下 eth0 是 r8168（PCIe），eth1 是 rk_gmac-dwmac（SoC）。
> 接口名称分配顺序不同，但物理对应关系相同。

## 下一步行动

1. **NPU 驱动问题**：需要找带 RKNPU 驱动的内核，或向定昌确认是否有带 NPU 的固件版本
2. **RKNN 用户态库**：librknnrt.so 需要与 OpenHarmony musl libc 兼容的版本
3. **HDC TCP 持久化**：重启后验证 TCP 连接是否生效
4. **SSH 访问**：通过 HDC shell 查看是否有 SSH 服务，或通过 Docker 启动 SSH
