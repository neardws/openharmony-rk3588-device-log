# YOLOv8n ncnn 推理 - OHOS ARM64 静态二进制

## 文件说明

- `yolov8_infer.cpp` - 主推理程序源码
- `yolov8_infer` (binary, not tracked) - 编译好的 ARM64 静态二进制

## 编译环境

```
Host:    Ubuntu x86_64
GCC:     aarch64-linux-gnu-g++ 13.3.0
ncnn:    latest (2025-03)
Target:  ARM64 aarch64, static, armv8.2-a+dotprod
```

## 编译步骤

```bash
# 1. 编译 ncnn（aarch64 静态库）
git clone --depth=1 https://github.com/Tencent/ncnn.git
cd ncnn && mkdir build-aarch64 && cd build-aarch64
cmake .. -GNinja \
  -DCMAKE_TOOLCHAIN_FILE=../toolchains/aarch64-linux-gnu.toolchain.cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DNCNN_BUILD_EXAMPLES=OFF -DNCNN_BUILD_TESTS=OFF \
  -DNCNN_VULKAN=OFF -DNCNN_SHARED_LIB=OFF \
  -DNCNN_ENABLE_LTO=ON -DBUILD_SHARED_LIBS=OFF
ninja -j$(nproc)

# 2. 编译推理程序
aarch64-linux-gnu-g++ -O2 -march=armv8.2-a+dotprod \
  -I/tmp/ncnn/src -I/tmp/ncnn/build-aarch64/src \
  yolov8_infer.cpp -o yolov8_infer -static \
  /tmp/ncnn/build-aarch64/src/libncnn.a -lgomp -lpthread -lm
```

## 模型转换

```bash
# 导出 ncnn 格式（需要 pnnx/ncnn Python 包）
pip install ultralytics pnnx ncnn
python3 -c "
from ultralytics import YOLO
YOLO('yolov8n.pt').export(format='ncnn', imgsz=640)
"
# 输出：yolov8n_ncnn_model/model.ncnn.param + model.ncnn.bin
```

## 部署到 OHOS

```bash
# 传输文件
hdc -t 192.168.31.93:5555 file send yolov8_infer /data/local/tmp/
hdc -t 192.168.31.93:5555 file send model.ncnn.param /data/local/tmp/
hdc -t 192.168.31.93:5555 file send model.ncnn.bin /data/local/tmp/

# 运行
hdc -t 192.168.31.93:5555 shell "chmod +x /data/local/tmp/yolov8_infer"
hdc -t 192.168.31.93:5555 shell "/data/local/tmp/yolov8_infer /data/local/tmp/model.ncnn.param /data/local/tmp/model.ncnn.bin"
```

## 注意事项

- 静态链接 glibc，在 musl libc 的 OHOS 上 dlopen 相关功能不可用（OpenMP offload 等）
- 但 OpenMP 并行计算本身可用（-fopenmp 编译，num_threads=4）
- 如遇 dlopen 错误，去掉 -lgomp 改用 -Wl,--no-as-needed 重新编译

## 模型信息

- YOLOv8n: 72层，3.15M 参数，8.7 GFLOPs
- 输入节点: `in0` (1,3,640,640)
- 输出节点: `out0` (84,8400) = [xywh(4) + classes(80), 8400 anchors]
- ncnn模型大小: 13MB

## 预期性能（RK3588 A76 4核）

- 预估: 50-150ms/帧 (6-20 FPS)
- 实测: 待板子运行后测量
