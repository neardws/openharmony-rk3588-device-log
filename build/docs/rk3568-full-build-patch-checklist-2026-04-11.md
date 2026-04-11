# RK3568 OpenHarmony Full Build Patch Checklist (2026-04-11)

Use this as the shortest repeatable checklist when reproducing the successful `rk3568@hihope` + `linux-5.10` build.

## 0. Build invocation

```bash
export PATH=/home/neardws/openharmony-build/bin:$PATH
export PYTHONHOME=/home/neardws/openharmony-build/ohos-src/prebuilts/python_llvm/linux-x86/3.11.4
export PYTHONPATH=/home/neardws/openharmony-build/ohos-src/prebuilts/python_llvm/linux-x86/3.11.4/lib/python3.11
cd /home/neardws/openharmony-build/ohos-src
./build.sh --product-name rk3568@hihope --gn-args linux_kernel_version="linux-5.10"
```

## 1. Add a fake `ccache`

Path:

- `/home/neardws/openharmony-build/bin/ccache`

Content:

```bash
#!/usr/bin/env bash
if [ $# -eq 0 ]; then
  exit 0
fi
case "$1" in
  -M|-F|-s|--show-stats|-z|--zero-stats|--version)
    exit 0
    ;;
  *)
    exec "$@"
    ;;
esac
```

## 2. Fix rk3568 kernel defconfig merge path variables

File:

- `device/board/hihope/rk3568/kernel/build_kernel.sh`

Expected lines:

```bash
DEFCONFIG_FORM_FILE=${HARMONY_CONFIG_PATH}/form/${KERNEL_FORM}_defconfig
DEFCONFIG_PROC_FILE=${DEVICE_CONFIG_PATH}/product/${KERNEL_PROD}_defconfig
```

## 3. Disable BTF in the kernel config used by the build

Required state:

```text
CONFIG_DEBUG_INFO_BTF=n
CONFIG_DEBUG_INFO_BTF_MODULES=n
```

## 4. Add `-mno-outline-atomics`

File:

- `kernel/linux/linux-5.10/arch/arm64/Makefile`

Add:

```make
KBUILD_CFLAGS += $(call cc-option,-mno-outline-atomics)
KBUILD_AFLAGS += $(call cc-option,-mno-outline-atomics)
```

## 5. Patch SPIR-V generator scripts to re-exec without `PYTHONHOME/PYTHONPATH`

Files:

- `third_party/spirv-tools/utils/generate_grammar_tables.py`
- `third_party/spirv-tools/utils/update_build_version.py`
- `third_party/spirv-tools/utils/generate_language_headers.py`
- `third_party/spirv-tools/utils/generate_registry_tables.py`

Insert near the top:

```python
if os.environ.get('PYTHONHOME') or os.environ.get('PYTHONPATH'):
    _env = dict(os.environ)
    _env.pop('PYTHONHOME', None)
    _env.pop('PYTHONPATH', None)
    os.execve(sys.executable, [sys.executable] + sys.argv, _env)
```

## 6. Patch `ets2panda` headers parser to use vendored PyYAML

File:

- `arkcompiler/ets_frontend/ets2panda/public/headers_parser/main.py`

Required behavior:

- compute repo root
- prepend `third_party/PyYAML/lib` to `sys.path`
- then import the existing parser helpers

This fixes:

```text
ModuleNotFoundError: No module named 'yaml'
```

## 7. Success markers

Search the log for:

```text
rk3568@hihope build success
=====build  successful=====
```

## 8. Final artifact checks

### Image files

- `out/rk3568/packages/phone/images/system.img`
- `out/rk3568/packages/phone/images/vendor.img`
- `out/rk3568/packages/phone/images/boot_linux.img`

### Media libraries

- `out/rk3568/packages/phone/vendor/lib/passthrough/indirect/librockchip_mpp.z.so`
- `out/rk3568/packages/phone/vendor/lib/passthrough/indirect/librockchip_vpu.z.so`
- `out/rk3568/packages/phone/vendor/lib/passthrough/indirect/libomxvpu_dec.z.so`
- `out/rk3568/packages/phone/vendor/lib/passthrough/indirect/libomxvpu_enc.z.so`
- `out/rk3568/packages/phone/vendor/lib/passthrough/indirect/libOMX_Pluginhw.z.so`
- `out/rk3568/packages/phone/vendor/lib/passthrough/indirect/librga.z.so`

## 9. What this proves

This proves the following build-time result:

- `rk3568@hihope` full build succeeds on `linux-5.10`
- Rockchip media decode/encode user-space libraries are included in final package outputs

This does **not** alone prove runtime hardware decode on the board. That still needs playback verification.
