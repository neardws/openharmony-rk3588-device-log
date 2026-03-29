# OpenHarmony 6.0 Release 编译方案（DC-A588-V04）

## 目标

为 DC-A588-V04（RK3588）板子编译定制 OpenHarmony 6.0 Release 系统镜像，
修复 NPU 驱动问题，集成 SSH 访问，支持 YOLOv8 CPU/NPU 推理。

## 环境

- 编译机：Ubuntu 24.04 x86_64
- 源码路径：`~/openharmony-build/ohos-src/`
- 源码分支：`OpenHarmony-6.0-Release`
- 磁盘空间：需要 ≥ 150GB（源码 ~66GB + 编译输出 ~50GB）

## 编译步骤

### 1. 安装依赖（已完成）

```bash
sudo apt install -y build-essential gcc-aarch64-linux-gnu python3-pip \
  python3-venv ruby cpio zip unzip curl wget ninja-build \
  libc6-dev-arm64-cross libssl-dev device-tree-compiler u-boot-tools

pip3 install --break-system-packages ohos-build
```

### 2. 获取源码（已完成）

```bash
mkdir -p ~/openharmony-build/ohos-src
cd ~/openharmony-build/ohos-src
~/bin/repo init -u https://gitcode.com/openharmony/manifest.git \
  -b OpenHarmony-6.0-Release -m default.xml --no-repo-verify
~/bin/repo sync -c -j8 --no-tags --no-clone-bundle
```

### 3. 创建板级配置（进行中）

DC-A588-V04 需要在以下路径创建板级配置：
- `device/board/hihope/rk3588/` — 板级构建配置
- `device/soc/rockchip/rk3588/kernel/dts/` — 设备树（关键）

参考目录结构见 `build/board-config/`。

### 4. 设备树适配（待完成）

**核心问题修复：**

当前厂商固件（4.1.7.5）NPU 不工作的根因：

```
gpio0-28 被 fd8b0030.pwm (PWM3) 占用
  → i2c@fea90000 (i2c-1) pinctrl_select 失败 → probe 失败
    → vdd_npu_s0 regulator 不存在
      → RKNPU driver → -EPROBE_DEFER → /dev/rknpu 不创建
```

**修复方法（在自定义 DTS 中）：**

```dts
/* 禁用 PWM3，释放 gpio0-28 给 i2c1 */
&pwm3 {
    status = "disabled";
    pinctrl-0 = <>;
};

/* 确保 i2c1 使用 i2c1m2（GPIO0_D4/D5）*/
&i2c1 {
    status = "okay";
    pinctrl-names = "default";
    pinctrl-0 = <&i2c1m2_xfer>;

    /* NPU 电源 PMIC */
    rk8602_npu: rk8602@42 {
        compatible = "rockchip,rk8602";
        reg = <0x42>;
        vin-supply = <&vcc5v0_sys>;
        regulator-compatible = "rk860x-reg";
        regulator-name = "vdd_npu_s0";
        regulator-min-microvolt = <550000>;
        regulator-max-microvolt = <950000>;
        regulator-ramp-delay = <2300>;
        rockchip,suspend-voltage-selector = <1>;
        regulator-boot-on;
        regulator-always-on;
    };
};

/* NPU 引用正确的电源 */
&rknpu {
    rknpu-supply = <&rk8602_npu>;
    status = "okay";
};
```

**参考资料：**
- Ubuntu 系统（NPU 正常工作）的 DTB：`hardware/ubuntu-dtb.bin`（已提取）
- OpenHarmony 官方 EVB DTS：`device/soc/rockchip/rk3588/kernel/arch/arm64/boot/dts/rockchip/rk3588-evb.dtsi`

### 5. 编译命令

```bash
cd ~/openharmony-build/ohos-src

# 设置编译目标
hb set
# 在菜单中选择: hihope -> rk3588

# 全量编译（约 2-3 小时）
hb build -f

# 输出目录
ls out/rk3588/
# system.img, vendor.img, boot.img, uboot.img 等
```

### 6. 刷机

将编译产物通过 RKDevTool 刷入板子：
- `MiniLoaderAll.bin` → Loader
- `parameter.txt` → 分区表
- `uboot.img` → uboot
- `boot.img` → boot
- `system.img` → system
- `vendor.img` → vendor
- `userdata.img` → data（可选）

## 编译产物预期大小

| 镜像文件 | 预期大小 |
|---------|---------|
| system.img | 2-3 GB |
| vendor.img | 500 MB |
| boot.img | 50 MB |
| 总计 | ~3.5 GB |

板子 eMMC 64GB，完全可以容纳。

## 进度追踪

- [x] 编译环境搭建（Ubuntu 24.04）
- [x] 依赖安装（hb, repo, aarch64-gcc 等）
- [x] 源码下载（6.0 Release，~66GB）
- [ ] 板级配置创建（device/board/hihope/rk3588）
- [ ] 设备树适配（NPU pinctrl 修复）
- [ ] 首次编译（验证构建系统）
- [ ] NPU 功能验证
- [ ] YOLOv8 推理集成测试

## 相关文档

- [NPU 驱动失败根因分析](../hardware/npu-probe-debug.md)
- [YOLOv8 ncnn CPU 推理方案](../deploy/ncnn/README.md)
- [OpenHarmony 官方 RK3588 SoC 仓库](https://gitcode.com/openharmony/device_soc_rockchip)
- [OpenHarmony 官方 HiHope 板级仓库](https://gitcode.com/openharmony/device_board_hihope)
