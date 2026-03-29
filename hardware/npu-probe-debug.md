# NPU Probe 失败根因分析（2026-03-29）

## 设备环境
- 板型：DC-A588-V04（RK3588）
- OS：OpenHarmony-4.1.7.5
- 内核：Linux 5.10.110-geaba2291341c-dirty（Jul 22 2025）

## 现象
- `/dev/rknpu` 不存在
- `ls /sys/class/rknpu/` 无输出
- RKNPU 驱动编译状态：`CONFIG_ROCKCHIP_RKNPU=y`（内置，非模块）

## 根因链路

```
i2c@fea90000 (i2c-1)
  → NO DRIVER（probe 失败，原因待查）
    → RK8602@42 (vdd_npu_s0 regulator) 不存在
      → RKNPU 驱动 probe 时请求 rknpu-supply → -EPROBE_DEFER
        → /dev/rknpu 不创建
```

## 详细诊断数据

### 1. NPU 设备节点
```
/sys/bus/platform/devices/fdab0000.npu/
  waiting_for_supplier: 1
  driver: 不存在（NO_DRV）
  uevent:
    OF_COMPATIBLE_0=rockchip,rk3588-rknpu
    MODALIAS=of:NnpuT(null)Crockchip,rk3588-rknpu
```

### 2. NPU Suppliers（全部 available）
```
supplier:platform:fd8d8000.power-management:power-controller  → available
supplier:platform:fdab9000.iommu                              → available
supplier:platform:firmware:scmi                               → available
```

### 3. Deferred Probe 队列
```
/sys/kernel/debug/devices_deferred:
  mtd_vendor_storage
  bt-sound
  fdab0000.npu   ← NPU 在此
```

### 4. Power Domain 状态
```
/sys/kernel/debug/pm_genpd/npu/current_state:    off-0
/sys/kernel/debug/pm_genpd/nputop/current_state: off-0
/sys/kernel/debug/pm_genpd/npu/sub_domains:      npu1\nnpu2
/sys/kernel/debug/pm_genpd/nputop/devices:       (空)
```

### 5. OPP 表
```
/sys/kernel/debug/opp/ 中无 NPU 条目（仅有 cpu0-7, gpu）
/proc/device-tree/npu-opp-table/ 存在，包含 opp-300M~1000M
npu-leakage@28 nvmem cell 存在
npu-thermal (thermal_zone6) 存在
NPU PVTM (fdaf0000.pvtm, compatible: rockchip,rk3588-npu-pvtm) 有驱动
```

### 6. PMIC 配置（关键问题）

**SPI PMIC（spi@feb20000/rk806single@0）- 仅一个 RK806：**
```
DCDC_REG1: vdd_gpu_s0
DCDC_REG2: vdd_cpu_lit_s0
DCDC_REG3: vdd_log_s0
DCDC_REG4: vdd_vdenc_s0
DCDC_REG5: vdd_ddr_s0
DCDC_REG6: vdd2_ddr_s3
DCDC_REG7: vdd_2v0_pldo_s3
DCDC_REG8: vcc_3v3_s3
DCDC_REG9: vddq_ddr_s0
DCDC_REG10: vcc_1v8_s3
← 无 vdd_npu_s0！
```

**I2C PMIC（i2c@fd880000，i2c-0）- 正常工作：**
```
rk8602@42 → vdd_cpu_big0_s0  ← DRIVER=rk860-regulator ✓
rk8603@43 → vdd_cpu_big1_s0  ← DRIVER=rk860-regulator ✓
```

**I2C PMIC（i2c@fea90000，i2c-1）- 问题所在：**
```
rk8602@42 → vdd_npu_s0  ← NO DRIVER（i2c 控制器 probe 失败）
```

### 7. i2c@fea90000 状态
```
/sys/bus/platform/devices/fea90000.i2c/
  waiting_for_supplier: 0（suppliers 均 available）
  driver: 不存在
  supplier:platform:pinctrl:    available
  supplier:platform:vcc5v0-sys: available
  uevent:
    OF_COMPATIBLE_0=rockchip,rk3588-i2c
    OF_COMPATIBLE_1=rockchip,rk3399-i2c
    OF_ALIAS_0=i2c1
```

手动 bind 尝试：
```sh
echo rk3x-i2c > /sys/bus/platform/devices/fea90000.i2c/driver_override
echo fea90000.i2c > /sys/bus/platform/drivers/rk3x-i2c/bind
# → exit:1（失败，原因不明）
```

### 8. 已绑定的 I2C 设备（rk3x-i2c 驱动）
```
fd880000.i2c  ← 正常（挂 CPU big0/big1 PMIC）
fead0000.i2c
fec80000.i2c
fec90000.i2c
← fea90000.i2c 不在列表中
```

## Regulator 列表（无 vdd_npu_s0）
```
系统注册的 41 个 regulator 中包括：
  vdd_gpu_s0, vdd_cpu_big0_s0, vdd_cpu_big1_s0, vdd_vdenc_s0, vdd_log_s0 ...
  ← 无 vdd_npu_s0
```

## 待查问题
1. `i2c@fea90000` (i2c-1) 为什么 probe 失败？
   - suppliers 全部 available（pinctrl + vcc5v0-sys）
   - clk 存在（clk_i2c1 @ 198MHz, pclk_i2c1 @ 100MHz）
   - compatible 正确（rockchip,rk3588-i2c / rockchip,rk3399-i2c）
   - rk3x-i2c 驱动已加载（其他 i2c 控制器正常）
   - 手动 bind 返回 exit:1

2. 是否缺少 pinmux 配置？（pinctrl-0 phandle = 0x0131）

3. 这是固件 BUG 还是 DTS 裁剪？

## 解决方案候选

### 方案 A：手动初始化 RK8602（通过 i2c-0 参考寄存器）
- 绕过 i2c@fea90000，直接用内核 i2c-dev 接口写 fea90000
- 风险：需要了解 RK8602 寄存器映射

### 方案 B：联系定昌要 NPU 支持版固件
- 当前固件 OpenHarmony-4.1.7.5 可能是裁剪版
- 定昌 Wiki 上有 OHOS 4.1.3 固件，未知是否包含完整 NPU 支持

### 方案 C：DTB overlay 修复
- 从 Ubuntu 系统（NPU 正常）提取工作的 DTB 片段
- 通过 configfs overlay 修复 i2c-1 的 pinctrl 配置

### 方案 D：换 Ubuntu 系统做 NPU 推理
- Ubuntu 系统 NPU 驱动正常工作
- 放弃 OHOS 上的 NPU，仅在 Ubuntu 部署 RKNN

## 参考
- rknpu2 驱动代码：https://github.com/airockchip/rknn-toolkit2
- NPU probe 相关：https://blog.csdn.net/River_ly/article/details/143187443
- 板子 compatible：rockchip,rk3588-evb7-v11 / rockchip,rk3588
- Ubuntu 内核（NPU 正常）：Linux 5.10.198，lhh2024@dcztl 编译
- OHOS 内核：Linux 5.10.110-geaba2291341c-dirty，Jul 22 2025
