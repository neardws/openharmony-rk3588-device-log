# RK3568 OpenHarmony 6.0 Full Build Fix Set (2026-04-11)

> Note: this repo is primarily tracking RK3588 device work, but this document captures a reusable OpenHarmony build repair set validated on `rk3568@hihope` with `linux-5.10`. The Python and toolchain issues are likely relevant to adjacent Rockchip OpenHarmony ports too.

## Target

- Source tree: `OpenHarmony-6.0-Release`
- Product: `rk3568@hihope`
- Kernel arg: `linux_kernel_version="linux-5.10"`
- Host: Ubuntu 24.04 x86_64

## Final Outcome

The following now succeed:

1. `./build.sh --product-name rk3568@hihope --gn-args linux_kernel_version="linux-5.10" --build-target kernel`
2. Full system build:

```bash
export PATH=/home/neardws/openharmony-build/bin:$PATH
export PYTHONHOME=/home/neardws/openharmony-build/ohos-src/prebuilts/python_llvm/linux-x86/3.11.4
export PYTHONPATH=/home/neardws/openharmony-build/ohos-src/prebuilts/python_llvm/linux-x86/3.11.4/lib/python3.11
cd /home/neardws/openharmony-build/ohos-src
./build.sh --product-name rk3568@hihope --gn-args linux_kernel_version="linux-5.10"
```

Successful end-of-build markers from `build-rk3568-full.log`:

- `[OHOS INFO]  rk3568@hihope build success`
- `=====build  successful=====`

## Root Causes Encountered

### 1. `ccache` hard dependency during kernel build

Some build paths assume `ccache` exists, even when caching is not required.

### 2. Broken variable expansion in rk3568 kernel merge config flow

`device/board/hihope/rk3568/kernel/build_kernel.sh` used malformed variable expansions for:

- `DEFCONFIG_FORM_FILE`
- `DEFCONFIG_PROC_FILE`

This broke merged defconfig resolution.

### 3. Kernel + toolchain incompatibilities on Linux 5.10

Two compiler-side issues had to be handled:

- BTF generation had to be disabled for this build path
- ARM64 outline atomics had to be suppressed with `-mno-outline-atomics`

### 4. Mixed Python runtime contamination

OpenHarmony build wrappers require bundled Python in some places, but some actions are executed by system Python 3.12. If `PYTHONHOME/PYTHONPATH` globally point at OHOS Python 3.11, system Python actions can fail with:

```text
AssertionError: SRE module mismatch
```

If `PYTHONHOME/PYTHONPATH` are removed globally, other early build stages can fail with:

```text
Fatal Python error: init_fs_encoding: failed to get the Python codec of the filesystem encoding
ModuleNotFoundError: No module named 'encodings'
```

### 5. SPIR-V generator scripts inherited the wrong Python environment

The failure path appeared under:

- `foundation/graphic/graphic_3d/lume/LumeShaderCompiler`
- `third_party/spirv-tools/utils/*.py`

### 6. `ets2panda` headers parser could not import `yaml`

Observed blocker:

```text
ACTION //arkcompiler/ets_frontend/ets2panda:libes2panda_public_parse_headers(...)
ModuleNotFoundError: No module named 'yaml'
```

The fix was to reuse the repo-vendored PyYAML instead of installing a system dependency.

## Minimal Reusable Fix Set

### A. Fake `ccache` shim

Create a wrapper at `/home/neardws/openharmony-build/bin/ccache` and prepend that directory to `PATH`:

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

Purpose: satisfy `ccache` probes while transparently executing the real compiler command.

### B. Fix rk3568 kernel defconfig merge variables

File:

- `device/board/hihope/rk3568/kernel/build_kernel.sh`

Confirmed relevant lines after fix:

- `DEFCONFIG_FORM_FILE=${HARMONY_CONFIG_PATH}/form/${KERNEL_FORM}_defconfig`
- `DEFCONFIG_PROC_FILE=${DEVICE_CONFIG_PATH}/product/${KERNEL_PROD}_defconfig`

### C. Disable BTF for this kernel build

Ensure these are unset / disabled in the final kernel config used for the build:

- `CONFIG_DEBUG_INFO_BTF=n`
- `CONFIG_DEBUG_INFO_BTF_MODULES=n`

This was a config-level fix rather than a durable source patch in the workspace.

### D. Disable ARM64 outline atomics

File:

- `kernel/linux/linux-5.10/arch/arm64/Makefile`

Added:

```make
KBUILD_CFLAGS += $(call cc-option,-mno-outline-atomics)
KBUILD_AFLAGS += $(call cc-option,-mno-outline-atomics)
```

### E. Keep global OHOS Python env, but locally sanitize system-Python actions

Working rule:

- keep global `PYTHONHOME/PYTHONPATH` for the overall OpenHarmony build
- remove them only inside Python scripts that are launched by system Python

### F. Patch SPIR-V generator scripts with self-reexec under clean env

Patched files:

- `third_party/spirv-tools/utils/generate_grammar_tables.py`
- `third_party/spirv-tools/utils/update_build_version.py`
- `third_party/spirv-tools/utils/generate_language_headers.py`
- `third_party/spirv-tools/utils/generate_registry_tables.py`

Pattern added near the top of each file:

```python
if os.environ.get('PYTHONHOME') or os.environ.get('PYTHONPATH'):
    _env = dict(os.environ)
    _env.pop('PYTHONHOME', None)
    _env.pop('PYTHONPATH', None)
    os.execve(sys.executable, [sys.executable] + sys.argv, _env)
```

### G. Patch `ets2panda` headers parser to find vendored PyYAML

File:

- `arkcompiler/ets_frontend/ets2panda/public/headers_parser/main.py`

Approach:

- compute repo root
- add `third_party/PyYAML/lib` to `sys.path`
- continue normal import flow

This avoids adding a host-level Python package dependency.

## Validation Path

### Kernel-only build

```bash
./build.sh --product-name rk3568@hihope \
  --gn-args linux_kernel_version="linux-5.10" \
  --build-target kernel
```

### Full build

```bash
export PATH=/home/neardws/openharmony-build/bin:$PATH
export PYTHONHOME=/home/neardws/openharmony-build/ohos-src/prebuilts/python_llvm/linux-x86/3.11.4
export PYTHONPATH=/home/neardws/openharmony-build/ohos-src/prebuilts/python_llvm/linux-x86/3.11.4/lib/python3.11
cd /home/neardws/openharmony-build/ohos-src
./build.sh --product-name rk3568@hihope --gn-args linux_kernel_version="linux-5.10"
```

## Final Artifact Checks

### Successful image outputs

Confirmed under `out/rk3568/packages/phone/images/`:

- `boot_linux.img`
- `system.img`
- `vendor.img`
- `updater.img`
- `userdata.img`
- `uboot.img`
- `MiniLoaderAll.bin`

### Confirmed media libraries staged into final package tree

Confirmed under `out/rk3568/packages/phone/vendor/lib/passthrough/indirect/`:

- `librockchip_mpp.z.so`
- `librockchip_vpu.z.so`
- `libomxvpu_dec.z.so`
- `libomxvpu_enc.z.so`
- `libOMX_Pluginhw.z.so`
- `librga.z.so`

### Desktop / UI stack indicators present

Confirmed in final package contents:

- `system/bin/render_service`
- `system/app/com.ohos.systemui/SystemUI.hap`
- `system/app/com.ohos.launcher/Launcher.hap`
- `system/etc/init/graphic.cfg`

## Important Scope Note

This document proves:

- the image builds successfully
- the Rockchip media user-space stack is integrated into the final package set

It does **not** by itself prove runtime playback on target hardware. For that, do a board-side validation pass with real H.264/H.265 streams and inspect codec/media logs.

## Non-blocking Warnings Seen at Build End

Build completion still emitted several warnings like:

```text
[WARNING]: sa module ... has no shlib_type="sa"
```

These were non-fatal for this build and did not prevent image generation.
