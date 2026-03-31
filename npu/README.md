# NPU 推理问题诊断报告

## 设备信息

| 项目 | 详情 |
|------|------|
| 开发板型号 | DC-A588-V04 |
| 主控芯片 | Rockchip RK3588 |
| 系统版本 | OpenHarmony 4.1.7.5 |
| 固件标识 | ZilOS（基于 OpenHarmony 4.1.3） |
| HDC 序列号 | 2c010e5344533753321002b19ba7ac00 |
| NPU 规格 | 6 TOPS（RK3588 内置，三核 NPU）|

---

## 问题描述

在该开发板上尝试运行 RKNN NPU 推理程序，初始化阶段即失败，无法加载模型和执行推理。

官方系统（ZilOS）和自行编译的 OpenHarmony 4.0 系统均复现，排除了软件编译层面的偶发问题。

---

## 诊断过程与结果

### 1. 内核配置：已编译 ✅

```
CONFIG_ROCKCHIP_RKNPU=y
CONFIG_ROCKCHIP_RKNPU_DEBUG_FS=y
CONFIG_ROCKCHIP_RKNPU_DRM_GEM=y
```

RKNPU 驱动已内建进内核（`=y`），不是模块加载问题。

### 2. 驱动注册：已注册 ✅

```
/sys/bus/platform/drivers/RKNPU   ← 驱动框架已注册
```

### 3. NPU 设备节点：不存在 ❌

```bash
$ ls /dev/rknpu*
ls: /dev/rknpu*: No such file or directory
```

`/dev/rknpu` 不存在，说明驱动 `probe()` 从未成功执行。

### 4. NPU 设备绑定状态：等待依赖 ❌

```bash
$ ls /sys/bus/platform/devices/fdab0000.npu/
driver_override
modalias
of_node
power
subsystem
supplier:platform:fd8d8000.power-management:power-controller
supplier:platform:fdab9000.iommu
supplier:platform:firmware:scmi
uevent
waiting_for_supplier          ← 关键：等待 supplier 就绪
```

`fdab0000.npu` 是 RK3588 NPU 的平台设备，当前状态为 `waiting_for_supplier`，即驱动 probe 被推迟，等待以下三个依赖就绪：

| Supplier | 作用 | 路径 |
|----------|------|------|
| `fd8d8000.power-management:power-controller` | NPU 电源域控制器 | 负责 VDD_NPU 供电 |
| `fdab9000.iommu` | IOMMU 单元 | NPU 内存访问隔离 |
| `firmware:scmi` | 系统控制管理接口 | 与 PMU 固件通信 |

### 5. RKNN Runtime 库：缺失 ❌

```bash
$ find /vendor /system -name 'librknnrt*'
（无输出）
```

系统中不存在 `librknnrt.so`，即使驱动修复后，上层推理程序也无法运行，需要额外部署。

### 6. dmesg 无 NPU 相关日志

```bash
$ dmesg | grep -i npu
（无输出）
```

驱动从未尝试初始化 NPU，与 `waiting_for_supplier` 一致。

---

## 根本原因分析

**`waiting_for_supplier` 是核心问题。**

Linux 设备驱动模型中，设备 probe 需要所有 supplier（依赖）先完成注册。当前 `fdab0000.npu` 的三个依赖中，至少有一个未能在内核启动阶段正确注册，导致 RKNPU 驱动的 probe 函数永远不会被调用，`/dev/rknpu` 也就无从创建。

最可能的原因（需工程师确认）：

1. **电源域 DTS 配置缺失或错误**  
   `fd8d8000.power-management` 是 RK3588 的 PMU/电源控制器，如果 DTS 中该节点配置不完整，NPU 的 `power-controller` supplier 就无法就绪。RK3588 NPU 有独立的电源域（`pd_npu`），供电轨为 VDD_NPU_0V75，需要正确的 regulator 配置。

2. **SCMI 固件未加载**  
   `firmware:scmi` 依赖 ATF（ARM Trusted Firmware）中的 SCMI 实现，如果该板的 ATF 版本不支持或未正确配置 SCMI，NPU 的电源管理接口就无法工作。

3. **IOMMU 未正确初始化**  
   `fdab9000.iommu` 是 NPU 专用 IOMMU，若 IOMMU DTS 配置缺失，NPU 无法安全访问内存。

---

## 给开发板工程师的问题

请工程师帮助确认以下内容：

1. 该板 BSP 包的 DTS 中，`fd8d8000.power-management`（PMU）节点是否完整配置？`pd_npu` 电源域是否启用？

2. ATF 固件是否支持 SCMI？版本号是多少？

3. `fdab9000.iommu`（NPU IOMMU）在 DTS 中是否有对应节点和 `status = "okay"`？

4. 是否有针对该板型（DC-A588-V04）的 BSP 补丁包，包含完整的 NPU 驱动适配？

5. 系统中是否应预置 `librknnrt.so`？如果是，应从哪个 BSP 包获取？

---

## 后续计划

- [ ] 等待工程师确认 DTS / ATF / BSP 情况
- [ ] 如工程师提供修复方案，重新编译内核验证
- [ ] 补充 `librknnrt.so` 并测试 RKNN 推理
- [ ] 若 OpenHarmony 无法解决，评估切换到 Buildroot Linux 运行 NPU
