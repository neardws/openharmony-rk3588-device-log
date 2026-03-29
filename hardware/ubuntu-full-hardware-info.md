# DC-A588-V04 完整硬件信息（Ubuntu 环境采集 2026-03-29）

> 通过 SSH 连接 Ubuntu 系统采集。此信息用于后续刷 OpenHarmony 前的硬件参考。

## 连接信息

- SSH: `ztl@192.168.31.175` (password: 123456)
- 网口：eth1（第二个 RJ45 口，PCIe网卡）

## 系统信息

| 项目 | 值 |
|------|------|
| 系统 | Ubuntu 22.04.4 LTS (Jammy) |
| 内核 | 5.10.198 #210 SMP (2025-02-25) |
| 编译者 | lhh2024@dcztl |
| GCC | aarch64-none-linux-gnu-gcc 10.3.1 (arm-10.29) |
| 架构 | aarch64 |
| 主机名 | 3588 |
| DTS model | `ztl, A588 Firefly roc-rk3399-pc PLUS` |
| DTS compatible | `rockchip,rk3588-evb7-v11` / `rockchip,rk3588` |
| SoC ID | 35881000 |
| Serial | 697ba025ffb7c7fb |
| OPTEE | revision 3.13 |
| MPP | 0d5f276d8, "change kernel for 22.04" |

## CPU

- 8 核 64 位
- CPU0-3: Cortex-A55 (0x412fd050)
- CPU4-7: Cortex-A76 (0x414fd0b0)
- BogoMIPS: 48.00
- 所有 CPU 启动于 EL2
- 支持: GICv3, Spectre-v4, Spectre-BHB, Virtualization, CRC32, LSE atomic

## 内存

- 物理内存: 8GB LPDDR5
- 可用: ~8GB
- DMA: [0x00200000-0xffffffff]
- Normal: [0x100000000-0x2ffffffff]
- CMA: 8MB @ 0x2ff800000
- Software IO TLB: 64MB @ 0xe9f00000-0xedf00000

## eMMC 分区表（GPT，58.25 GiB）

| 分区 | 起点扇区 | 大小 | PARTLABEL | FSTYPE | LABEL | 备注 |
|------|---------|------|-----------|--------|-------|------|
| mmcblk0p1 | 16384 | 4M | uboot | - | - | U-Boot |
| mmcblk0p2 | 24576 | 4M | misc | - | - | 杂项 |
| mmcblk0p3 | 32768 | 2G | boot | - | boot | 内核/启动 |
| mmcblk0p4 | 4227072 | 128M | recovery | - | - | 恢复 |
| mmcblk0p5 | 4489216 | 32M | backup | - | backup | 备份 |
| mmcblk0p6 | 4554752 | 64M | userdata | ext4 | userdata | 用户数据 |
| mmcblk0p7 | 4685824 | 128M | oem | ext4 | oem | OEM |
| mmcblk0p8 | 4947968 | 55.9G | rootfs | ext4 | rootfs | 根文件系统 |

- mmcblk0boot0: 4M (boot0)
- mmcblk0boot1: 4M (boot1)
- mmcblk0rpmb: 16M (RPMB)

### PARTUUID 清单

```
uboot:     59720000-0000-4918-8000-788d00003f6d
misc:      79200000-0000-472e-8000-34a50000259c
boot:      7a3f0000-0000-446a-8000-702f00006273
recovery:  12570000-0000-4b0b-8000-7b5a00001c1f
backup:    59700000-0000-4f37-8000-50c700006d43
userdata:  7e690000-0000-4611-8000-50cb00006232
oem:       e84d0000-0000-476f-8000-62cf00003ed6
rootfs:    614e0000-0000-4b53-8000-1d28000054a9
```

### Kernel cmdline

```
storagemedia=emmc androidboot.storagemedia=emmc androidboot.mode=normal
androidboot.verifiedbootstate=orange androidboot.serialno=697ba025ffb7c7fb
rw rootwait earlycon=uart8250,mmio32,0xfeb50000 console=ttyFIQ0
irqchip.gicv3_pseudo_nmi=0 root=PARTUUID=614e0000-0000
rcupdate.rcu_expedited=1 rcu_nocbs=all
```

## 网络接口

### eth0 — 千兆以太网（PCIe RTL8168）

- 驱动: r8168 v8.049.02-NAPI
- 芯片: Realtek RTL8168 (PCIe, [10ec:8168])
- PHY: RTL8211F Gigabit Ethernet
- 接口: RGMII_RXID
- MAC (永久): 62:5d:ef:f6:78:eb
- MAC (当前): 5e:67:7a:b6:68:f7
- 状态: DOWN（未接线）
- 对应: /sys/class/net/eth0 → fe1b0000.ethernet (SoC内建GMAC)

### eth1 — 千兆以太网（PCIe RTL8125B）

- 芯片: Realtek RTL8125B (PCIe, [10ec:b852])
- 驱动: r8168
- MAC: 96:eb:88:67:4d:92 (permaddr: 62:5d:ef:f6:78:eb)
- 状态: UP, 192.168.31.175/24
- 对应: /sys/class/net/eth1 → 0003:31:00.0 (PCIe网卡)

### Wi-Fi

- 模块: AP6275P (Broadcom BCM43752)
- 驱动: bcmdhd (wlan=r892223-20220701-3)
- ⚠️ 加载失败：`pcie_register_driver failed`，Wi-Fi 当前不可用
- 固件版本: 101.10.361.20
- 接口类型: PCIe

### Bluetooth

- 驱动已加载: HCI UART + rfkill_rk
- 芯片组: AP6275P (与 Wi-Fi 一体)
- GPIO: reset=22, wake=21, host_wake_irq=0

## NPU / RKNPU

- DTS 节点: `npu@fdab0000` → compatible: `rockchip,rk3588-rknpu`
- status: `okay`
- 驱动: rknpu 0.9.3 (20231121)
- iommu: enabled
- bin=0, leakage=8, pvtm=873, avs=0
- 供电: vdd_npu_s0 (由 vcc4v0_sys 供电)
- ⚠️ power_model 初始化失败（非致命）
- ⚠️ **未安装 RKNN 用户态运行时**（无 librknnrt.so, 无 rknn_server）

## GPU (Mali-G610)

- 设备: /dev/mali0
- 驱动: bifrost g18p0-01eac0
- GPU ID: arch 10.8.6 r0p0
- bin=0, leakage=14, pvtm=861
- 供电: vdd_gpu_s0 (由 vcc4v0_sys 供电)
- Mali firmware 已加载 (git_sha: ee476db4)

## 显示 (VOP2)

### 输出端口

| 端口 | 类型 | DRM | 状态 | 分辨率 |
|------|------|-----|------|--------|
| vp0 | DSI (MIPI) | card0-DSI-1 | connected | 800x1280p60 (竖屏) |
| vp1 | HDMI OUT 1 | card0-HDMI-A-2 | **connected** | **3840x2160p60** |
| vp2 | HDMI OUT 0 | card0-HDMI-A-1 | disconnected | - |
| vp3 | - | card0-Writeback-1 | unknown | - |

- HDMI PHY: hdptx @ fed60000 (HDMI0) + fed70000 (HDMI1)
- HDMI HDCP: 已注册
- DSI: dw-mipi-dsi2 @ fde20000
- GM8775 芯片初始化 (MIPI转接芯片，ztl定制)
- HDMI-A-2 存在 i2c read error（EDID读取失败但不影响显示）

## 媒体编解码 (MPP)

- mpp_service: 已加载
- VDPU1: fdb51000 (avsd-plus)
- VDPU2: fdb50400
- VEPU2: fdb50000 + JPEG编码 fdba0000/fdba4000/fdba8000/fdbac000
- RKVDEC2: fdc38100 + fdc48100 (双核视频解码)
- RKVENC2: fdbd0000 + fdbe0000 (双核视频编码)
- AV1解码: fdc70000
- JPEG解码: fdb90000
- IEP2: fdbb0000
- RGA2/3: fdb60000 + fdb70000 (2D图形加速)

## 摄像头接口

- CSI2-DCPHY0/1: 已探测
- CSI2-DPHY0/1 + HW: 已探测
- /dev/video0: 可用
- rkaiq_3A 服务: 已启动（Rockchip camera engine）

## HDMI IN

- 设备: fdee0000.hdmirx-controller
- 驱动: rk_hdmirx
- 已注册音频设备: card0

## 音频

- ES8323/ES8388: 7-0011 (codec, 有些写入错误)
- 声卡列表:
  - #0: rockchip,hdmiin
  - #1: rockchip-es8388
  - #2: rockchip-hdmi0
  - #3: rockchip-hdmi1

## USB 拓扑

- xhci-hcd.3.auto: USB 3.0 Host (fc400000) — 对应下层 Type-A
- xhci-hcd.4.auto: USB 3.0 Host (fc000000) — 对应上层 OTG(当前Host模式)
  - 接有: SIGMACHIP USB Mouse
- xhci-hcd.14.auto: USB 3.0 (otg gadget 模式?)
- ehci-platform: fc800000 + fc880000 (USB 2.0 EHCI)
- ohci-platform: fc840000 + fc8c0000 (USB 1.1 OHCI)
- 内部 USB Hub: 1a40:0101 (USB 2.0 Hub, 4 ports)

## 串口

| 设备 | 基地址 | 中断 | 波特率 | 推测对应 |
|------|--------|------|--------|---------|
| ttyS0 | fd890000 | 27 | 1500000 | ttys0 (TTL) |
| ttyS3 | feb60000 | 87 | 1500000 | ttys3 (TTL) |
| ttyS7 | feba0000 | 88 | 1500000 | ttys7 (TTL) |
| ttyS8 | febb0000 | 89 | 1500000 | ttys8 (TTL) |
| ttyFIQ0 | - | - | - | 调试串口 (ttys2 映射) |

- 调试串口: earlycon=uart8250,mmio32,0xfeb50000, console=ttyFIQ0

## 其他硬件

- SATA: fe210000 (AHCI, link down — 未接盘)
- RTC: hym8563 @ 6-0051
- PMIC: RK806 @ SPI2
- CAN: fea50000 + fea60000 + fea70000
- cryptodev: 1.12
- zram: 已配置

## DTS compatible 信息（用于 OpenHarmony 移植参考）

```
板级: rockchip,rk3588-evb7-v11
芯片: rockchip,rk3588
```

## DTB 备份

已从运行系统提取 DTB：
- `hardware/dtb/board-live.dtb` (274432 bytes)

## 完整 dmesg 日志

见: `logs/ubuntu-dmesg.log`

## 刷 OpenHarmony 前的关键信息总结

1. **分区表**: GPT, 8个分区, rootfs PARTUUID=614e0000-0000
2. **网卡**: eth0=RTL8211F(SoC GMAC), eth1=RTL8125B(PCIe)
3. **Wi-Fi**: AP6275P/BCM43752 (PCIe, 驱动有问题)
4. **显示**: DSI 800x1280(竖屏) + HDMI-A-2 3840x2160
5. **NPU**: rknpu 0.9.3 驱动已加载, 需安装用户态库
6. **DTB compatible**: rockchip,rk3588-evb7-v11
7. **调试串口**: 0xfeb50000, console=ttyFIQ0, 波特率 1500000
8. **GM8775**: MIPI 转接芯片 (ztl 定制)
