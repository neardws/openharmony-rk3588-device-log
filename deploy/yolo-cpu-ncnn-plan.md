# YOLO CPU 推理部署方案（ncnn 静态编译）

## 背景

OpenHarmony 使用 musl libc，Python pip 包（ultralytics/onnxruntime）动态链接 glibc，
无法直接在 OHOS 上运行。Docker 方案已验证可用但有稳定性风险（曾导致系统崩溃）。

**选定方案：ncnn 静态编译 ARM64 独立 binary，直接部署到 OHOS。**

## 工具链

- 编译机：Ubuntu x86_64（192.168.31.175 旁的开发机）
- 目标：ARM64 aarch64-linux-musl（静态链接）
- 框架：[ncnn](https://github.com/Tencent/ncnn) - 轻量推理，支持静态编译，ARM NEON 优化

## 编译步骤

### 1. 安装交叉编译工具链
```bash
apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu cmake ninja-build
```

### 2. 编译 ncnn（静态，关闭 Vulkan）
```bash
git clone --depth=1 https://github.com/Tencent/ncnn.git /tmp/ncnn
cd /tmp/ncnn
mkdir build-aarch64 && cd build-aarch64
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=../toolchains/aarch64-linux-gnu.toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DNCNN_BUILD_EXAMPLES=OFF \
  -DNCNN_BUILD_TESTS=OFF \
  -DNCNN_VULKAN=OFF \
  -DNCNN_SHARED_LIB=OFF \
  -DNCNN_ENABLE_LTO=ON \
  -DBUILD_SHARED_LIBS=OFF
make -j$(nproc)
```

### 3. 准备 YOLOv8n 模型（ONNX → ncnn）
```bash
# 在 x86 Python 环境中导出
pip install ultralytics
python3 -c "from ultralytics import YOLO; m=YOLO('yolov8n.pt'); m.export(format='ncnn')"
# 或者直接下载官方 ncnn 格式
```

### 4. 编译推理程序（静态链接）
参考 ncnn yolov8 example，静态链接 libncnn.a

### 5. 部署到 OHOS
```bash
hdc file send yolov8_ncnn /data/local/tmp/
hdc file send yolov8n.ncnn /data/local/tmp/
hdc shell "chmod +x /data/local/tmp/yolov8_ncnn && /data/local/tmp/yolov8_ncnn test.jpg"
```

## 预期性能

RK3588 Cortex-A76 4核 @ 2.4GHz：
- YOLOv8n (640x640): 预计 ~5-15 FPS（CPU only）
- 对比 NPU：~30-60 FPS（如果 NPU 能启用）

## 当前状态

- [x] 方案选定
- [ ] ncnn 编译中
- [ ] 模型转换
- [ ] 推理程序编写
- [ ] 板子部署测试

## NPU 进展（暂缓）

见 `hardware/npu-probe-debug.md`。

根因：gpio0-28 被 `remotectl-pwm`（IR 遥控 PWM）占用，i2c@fea90000 无法 probe，
导致 vdd_npu_s0 regulator 不存在，RKNPU 驱动永久 defer。

已尝试方案：
- setenforce 0 + unbind remotectl-pwm → 失败（driver 无 unbind 节点）
- devmem 直接写 PMU GRF 寄存器 → SIGBUS（内核限制 /dev/mem 访问范围）
- gpio export 抢占 → pin 28 仍被 pwm 占用

下一步（待做）：
- 编写内核模块通过 pinctrl API 强制切换 mux
- 或联系定昌要带 NPU 的完整固件
