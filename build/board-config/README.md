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
