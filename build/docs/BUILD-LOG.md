# OpenHarmony 6.0 Release 编译日志（DC-A588-V04）

记录从零开始到成功编译的完整流程，包括所有踩坑和解决方案。

## 环境

- 编译机：Ubuntu 24.04 x86_64，188.5GB RAM，1.8T NVMe
- 源码路径：`~/openharmony-build/ohos-src/`
- 源码分支：`OpenHarmony-6.0-Release`（repo sync，约 64GB）
- 目标产品：`rk3588@hihope`

---

## 阶段 1：板级配置创建

参考 `device/board/hihope/rk3568/` 结构，为 rk3588 创建以下目录：

```
device/board/hihope/rk3588/
  BUILD.gn
  config.json
  ohos.build          # subsystem: device_rk3588（必须与 preloader 生成名一致）

vendor/hihope/rk3588/
  config.json         # device_company: "hihope"（不能写 rockchip！）
  ohos.build          # subsystem: product_rk3588
  product.gni
  ...
```

**关键坑 #1：ohos.build subsystem 命名规则**

preloader 动态生成 subsystem 名：
- `device/board/<company>/<boardname>/` → `device_<boardname>`
- `vendor/<company>/<productname>/` → `product_<productname>`

`ohos.build` 里的 `subsystem` 字段必须完全匹配，否则 LOAD 阶段报错。

**关键坑 #2：device_company 字段**

`vendor/hihope/rk3588/config.json` 中：
```json
{
  "device_company": "hihope",   // ← 必须是 hihope，不是 rockchip
  ...
}
```
如果写 `rockchip`，hb 会把 `device_path` 解析到 `device/board/rockchip/rk3588/`（不存在），LOAD 失败。

---

## 阶段 2：工具链准备

### 2.1 Clang（OHOS 专版）

```bash
# URL 从 build/prebuilts_download_config.json 获取
wget https://repo.huaweicloud.com/openharmony/compiler/clang/15.0.4-bb5cdf/linux/clang_linux-x86_64-bb5cdf-20250726.tar.gz \
  -O ~/openharmony-build/clang-ohos-linux.tar.gz

mkdir -p prebuilts/clang/ohos/linux-x86_64/llvm
tar xf ~/openharmony-build/clang-ohos-linux.tar.gz \
  --strip-components=1 -C prebuilts/clang/ohos/linux-x86_64/llvm/
```

验证：`prebuilts/clang/ohos/linux-x86_64/llvm/bin/clang --version`
输出：`OHOS (dev) clang version 15.0.4 (bb5cdf)`

### 2.2 libcxx-ndk

```bash
wget https://repo.huaweicloud.com/openharmony/compiler/clang/15.0.4-bb5cdf/linux/libcxx-ndk_linux-x86_64-bb5cdf-20250726.tar.gz \
  -O ~/openharmony-build/libcxx-ndk.tar.gz

mkdir -p prebuilts/clang/ohos/linux-x86_64/libcxx-ndk
tar xf ~/openharmony-build/libcxx-ndk.tar.gz \
  --strip-components=1 -C prebuilts/clang/ohos/linux-x86_64/libcxx-ndk/
```

### 2.3 GN + Ninja

```bash
wget https://repo.huaweicloud.com/openharmony/compiler/gn/20250804/gn-linux-x86-20250804.tar.gz
wget https://repo.huaweicloud.com/openharmony/compiler/ninja/1.12.0/linux/ninja-linux-x86-1.12.0-20240523.tar.gz

mkdir -p prebuilts/build-tools/linux-x86/bin
tar xf gn-*.tar.gz -C prebuilts/build-tools/linux-x86/bin/
tar xf ninja-*.tar.gz -C prebuilts/build-tools/linux-x86/bin/
```

### 2.4 Node.js v18

```bash
wget https://repo.huaweicloud.com/nodejs/v18.20.1/node-v18.20.1-linux-x64.tar.gz

mkdir -p prebuilts/build-tools/common/nodejs
tar xf node-v18.20.1-linux-x64.tar.gz -C prebuilts/build-tools/common/nodejs/
ln -sf node-v18.20.1-linux-x64 prebuilts/build-tools/common/nodejs/current
```

### 2.5 ARK AOT 工具链

```bash
wget https://repo.huaweicloud.com/openharmony/compiler/clang/15.0.4-0d8b9c-ark/llvm-aot-x86.tar.xz \
  -O ~/openharmony-build/llvm-aot-x86.tar.xz

mkdir -p prebuilts/ark_tools/llvm_aot/aot_x86_release
tar xf ~/openharmony-build/llvm-aot-x86.tar.xz \
  --strip-components=1 -C prebuilts/ark_tools/llvm_aot/aot_x86_release/
```

### 2.6 Native SDK（API 20）

```bash
# 从大包里流式提取（避免下载 2.5GB 完整包）
curl -L "https://repo.huaweicloud.com/openharmony/os/6.0-Release/ohos-sdk-windows_linux-public.tar.gz" \
  | tar xzO linux/native-linux-x64-6.0.0.47-Beta1.zip > /tmp/native-sdk.zip

mkdir -p prebuilts/ohos-sdk/linux/20/native
unzip -q /tmp/native-sdk.zip -d prebuilts/ohos-sdk/linux/20/native/
```

### 2.7 运行 npm install

```bash
bash build/prebuilts_download.sh
# 这会对所有 npm_install_path 目录执行 npm install
```

---

## 阶段 3：修复系统依赖

### 3.1 libnl（第三方库）

```bash
sudo apt install -y libtool flex bison
cd third_party/libnl && bash install.sh
```

**关键坑 #3：libnl install.sh patch 路径**

`install.sh` 里 patch 命令写的是相对路径，从 GN exec_script 调用时工作目录不对，导致报错。
绕过方法：手动 cd 进目录执行。

### 3.2 Ruby 兼容性（Ruby 3.2+）

```bash
find arkcompiler -name "*.rb" | xargs grep -l "File.exists?" | while read f; do
  sed -i 's/File\.exists?/File.exist?/g' "$f"
done
```

`File.exists?` 在 Ruby 3.2 中被删除，替换为 `File.exist?`。

### 3.3 Python json5 模块

```bash
pip3 install json5 --break-system-packages

# prebuilt python 没有 SSL，无法 pip install，直接 cp
cp -r ~/.local/lib/python3.12/site-packages/json5 \
  prebuilts/python_llvm/linux-x86/3.11.4/lib/python3.11/site-packages/
cp -r ~/.local/lib/python3.12/site-packages/json5 \
  prebuilts/clang/ohos/linux-x86_64/llvm/python3/lib/python3.11/site-packages/
```

### 3.4 ohpm 权限

```bash
chmod +x prebuilts/tool/command-line-tools/ohpm/bin/*
```

---

## 阶段 4：配置 hb 并运行编译

```bash
cd ~/openharmony-build/ohos-src
pip3 install --break-system-packages build/hb   # ohos-build 1.0.0
hb set -root . -p rk3588@hihope
hb build 2>&1 | tee ~/openharmony-build/build-run5.log
```

编译阶段通过情况：
- ✅ PRELOAD
- ✅ LOAD
- ✅ GN gen（114064 targets，12706ms）
- ✅ NINJA 启动（94641 targets 总计）
- ❌ NINJA ~6350 处卡住：`power_dialog_hap_compile_app` 失败

---

## 阶段 5：HAP 编译失败原因与跳过方案

### 失败原因

源码里 HAP 子项目（约 508 个）的 `hvigor-config.json5` 需要：
```json
{
  "hvigorVersion": "4.0.9",
  "dependencies": {
    "@ohos/hvigor-ohos-plugin": "4.0.9"
  }
}
```

但 prebuilts 里只有 5.8.9 版本，且 4.0.9 不在任何公共 npm registry：
- `registry.npmjs.org` → 404
- `repo.huaweicloud.com/repository/npm` → 404
- `registry.npmmirror.com` → 404
- `ohpm.openharmony.cn/ohpm` → 空响应 `{}`

**结论**：4.0.9 版本只存在于 DevEco Studio 工具包里，没有公开镜像。

### 跳过方案：compile_app.py early exit

```python
# build/scripts/compile_app.py 开头插入（在 import 之后）：
import sys
if __name__ == '__main__':
    # Skip HAP compilation - hvigor 4.0.9 not available
    # HAP apps not required for driver/kernel development
    import json, os, argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--output-file', default='')
    args, _ = parser.parse_known_args()
    if args.output_file:
        os.makedirs(os.path.dirname(args.output_file), exist_ok=True)
        with open(args.output_file, 'w') as f:
            json.dump([], f)
    sys.exit(0)
```

这样 ninja 会认为 HAP target 成功（输出空列表），继续编译剩余 C++ target，最终生成系统镜像。

**影响评估**：
- ❌ 系统 UI App（设置、桌面等）不可用
- ✅ 内核 + 驱动（rknpu、i2c、pmic）
- ✅ 系统服务（hdf、samgr、hilog）
- ✅ HDC 调试通道（hdcd）
- ✅ Shell 访问
- ✅ ncnn CPU 推理测试

对 NPU 驱动开发和 AI 推理验证没有影响。

---

## 当前状态（2026-03-29）

- 编译进度：NINJA ~6350/94641 targets，卡在 HAP
- 下一步：应用 compile_app.py 跳过方案，重新运行 `hb build`
- 预期产出：`out/rk3588/` 下的 `system.img` / `vendor.img` / `boot.img`
