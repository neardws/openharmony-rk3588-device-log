# 板级配置文件（DC-A588-V04 / RK3588）

## 目录结构

```
board-config/
└── device/
    └── board/
        └── hihope/
            └── rk3588/
                ├── BUILD.gn          # 构建配置
                ├── config.json       # hb 编译目标
                └── kernel/
                    └── dts/
                        └── dc-a588-v04.dts   # 板级设备树（核心）
```

## 使用方法

将 `device/board/hihope/rk3588/` 目录复制到 OpenHarmony 源码树的对应位置：

```bash
cp -r board-config/device/board/hihope/rk3588 \
  ~/openharmony-build/ohos-src/device/board/hihope/
```

## 关键修改说明

### 设备树修复（NPU 驱动问题）

`dc-a588-v04.dts` 相对于上游 EVB 设备树的关键修改：

1. **禁用 PWM3**
   - 原因：`fd8b0030.pwm` (PWM3) 占用了 `gpio0-28`，导致 `i2c@fea90000` (i2c-1) pinctrl 失败
   - 修改：`&pwm3 { status = "disabled"; }`

2. **启用 i2c1 (fea90000)**
   - 使用 `i2c1m2_xfer` pinmux（GPIO0_D4/D5）
   - 这使得挂载在 i2c1 上的 RK8602 PMIC 可以正常初始化

3. **vdd_npu_s0 电源**
   - 在 i2c1 上添加 `rk8602@42`（NPU 电源 PMIC）
   - NPU 驱动之前因找不到这个 regulator 而一直 EPROBE_DEFER

4. **启用 NPU**
   - `&rknpu { rknpu-supply = <&rk8602_npu>; status = "okay"; }`

### 验证状态

- [ ] 编译通过
- [ ] 刷机验证
- [ ] `/dev/rknpu` 节点存在
- [ ] NPU 推理测试（YOLOv8）

## TODO

待从 Ubuntu 系统（NPU 工作正常）提取完整 DTB 后：
1. 对比 `vdd_npu_s0` 相关节点的实际配置
2. 验证 PWM3 的实际用途（风扇/背光？）
3. 补充其他板子特有外设配置（SATA, CAN, RS485 等）

## Ubuntu 验证数据（2026-03-29）

已从 Ubuntu 5.10.198 系统（NPU 工作）提取运行时 DTB，确认以下信息：

### i2c@fea90000（i2c1）实际配置
```
pinctrl-0 = <i2c1m2-xfer>;  // GPIO0_D4(pin28)/D5(pin29), func9
status = "okay";
子设备: rk8602@42 (vdd_npu_s0, 550mV~950mV)
```

### PWM3 状态
```
pwm@fd8b0030: status = "okay"
```
**注意：Ubuntu DTB 中 PWM3 是 okay 的！**  
这意味着 OHOS 厂商固件中 gpio0-28 被 PWM3 占用的原因是**引脚 mux 配置不同**，
不是 PWM3 本身占用，而是 OHOS 固件的 pinctrl 配置把 gpio0-28 给了 PWM3 而非 i2c1m2。
修复方向：确保 i2c1 pinctrl 指向 `i2c1m2_xfer`（不与 PWM3 冲突）。

### NPU 状态
- rknpu 0.9.3 初始化成功（但有 can't request region 警告，非致命）
- supplier: `i2c:1-0042`（rk8602 成功 probe）

### 参考文件
- `hardware/ubuntu-running-5.10.198.dtb` — 完整 DTB（268KB）
- `hardware/ubuntu-running-5.10.198.dts` — 反编译 DTS（322KB）
