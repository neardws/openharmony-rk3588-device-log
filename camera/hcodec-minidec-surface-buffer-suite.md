# `hcodec_minidec` surface / buffer 设备侧测试脚本与日志采集

> 目标：把当前 `hcodec_minidec` 的三条关键验证路径固化成可重复执行的一套流程，减少每次上板都重新拼命令。
>
> 覆盖路径：
>
> 1. **最小 surface 验证**
> 2. **buffer sync 吞吐基线**
> 3. **buffer async 吞吐对比**

---

## 1. 相关文件

- 设备侧脚本：`camera/scripts/hcodec_minidec_suite.sh`
- sample 源码：`/home/neardws/openharmony-build/ohos-src/foundation/multimedia/av_codec/test/unittest/hcodec_test/demo/hcodec_minidec.cpp`
- 主日志目录（设备）：`/data/local/tmp/hcodec_logs/<run_id>/`
- 建议拉回目录（主机）：`/home/neardws/openharmony-build/logs/hcodec/<run_id>/`

---

## 2. 设备侧脚本做了什么

脚本会自动：

- 设置 `LD_LIBRARY_PATH`
- 记录当前二进制、输入文件、位数等环境信息
- 在每个 case 前后采集：
  - `hidumper -s 3002`
  - `hidumper -s 3011`
  - `ps -ef`
- 运行以下 case：
  - `surface_minimal`
  - `buffer_sync`
  - `buffer_async`
- 为每个 case 生成单独日志
- 汇总关键行到 `summary.txt`

默认输出目录结构类似：

```text
/data/local/tmp/hcodec_logs/20260416_113000/
  suite.log
  results.txt
  summary.txt
  surface_minimal.log
  buffer_sync.log
  buffer_async.log
  hidumper_3002_before.txt
  hidumper_3011_before.txt
  hidumper_3002_surface_minimal.txt
  hidumper_3011_surface_minimal.txt
  ...
```

---

## 3. 下发脚本

### 3.1 直接有 `hdc` 的情况

```bash
hdc file send openharmony-rk3588-device-log/camera/scripts/hcodec_minidec_suite.sh /data/local/tmp/hcodec_minidec_suite.sh
hdc shell "chmod +x /data/local/tmp/hcodec_minidec_suite.sh"
```

### 3.2 如果仍走 Windows 中转机

沿用你现有的 `ssh -> Windows -> hdc.exe` 方式执行同样的 `file send` 和 `shell chmod` 即可。

当前记忆里的中转信息是：

- Windows relay: `neardws@100.124.185.121`
- `hdc.exe`: `C:\Users\neardws\Desktop\hdc\hdc.exe`

这里不强行固化成单条 SSH 命令，是因为不同 Windows OpenSSH / shell quoting 差异比较大，容易写成看起来对、实际不好用的假命令。设备侧 runner 已经固定，host 侧只要用你现有可用的 `hdc` 链路下发即可。

---

## 4. 运行命令

### 4.1 跑完整套件（推荐）

```bash
hdc shell "/data/local/tmp/hcodec_minidec_suite.sh /data/local/tmp/<input>.h264 <width> <height> all"
```

例如：

```bash
hdc shell "/data/local/tmp/hcodec_minidec_suite.sh /data/local/tmp/test-h264-real-lauterbrunnen-1920x1080-high-60fps-10m18s.h264 1920 1080 all"
```

这会依次跑：

1. `surface_minimal`
2. `buffer_sync`
3. `buffer_async`

### 4.2 只跑最小 surface 验证

```bash
hdc shell "/data/local/tmp/hcodec_minidec_suite.sh /data/local/tmp/<input>.h264 <width> <height> surface"
```

### 4.3 只跑 buffer 对比

```bash
hdc shell "/data/local/tmp/hcodec_minidec_suite.sh /data/local/tmp/<input>.h264 <width> <height> buffer"
```

### 4.4 单独跑 buffer sync / async

```bash
hdc shell "/data/local/tmp/hcodec_minidec_suite.sh /data/local/tmp/<input>.h264 <width> <height> buffer-sync"
hdc shell "/data/local/tmp/hcodec_minidec_suite.sh /data/local/tmp/<input>.h264 <width> <height> buffer-async"
```

---

## 5. 可选环境变量

脚本支持在执行前覆盖这些变量：

```bash
HCODEC_MINIDEC_BIN=/data/local/tmp/hcodec_minidec
HCODEC_LOG_ROOT=/data/local/tmp/hcodec_logs
HCODEC_RUN_ID=my_run_name
HCODEC_MINIDEC_FRAME_RATE=60
HCODEC_MINIDEC_PROGRESS_MS=1000
HCODEC_MINIDEC_LOG_INTERVAL=0
```

例如：

```bash
hdc shell "HCODEC_RUN_ID=lauterbrunnen_1080p60_a1 HCODEC_MINIDEC_FRAME_RATE=60 /data/local/tmp/hcodec_minidec_suite.sh /data/local/tmp/test-h264-real-lauterbrunnen-1920x1080-high-60fps-10m18s.h264 1920 1080 all"
```

---

## 6. 日志采集命令

### 6.1 先列出设备上的 run 目录

```bash
hdc shell "ls -1 /data/local/tmp/hcodec_logs"
```

### 6.2 拉回指定 run

```bash
mkdir -p /home/neardws/openharmony-build/logs/hcodec
hdc file recv /data/local/tmp/hcodec_logs/<run_id> /home/neardws/openharmony-build/logs/hcodec/
```

### 6.3 主机侧快速看 summary

```bash
cat /home/neardws/openharmony-build/logs/hcodec/<run_id>/summary.txt
```

### 6.4 快速比较三个 case 的 summary

```bash
rg -n "^==|^\[summary\]|^\[codec-error\]|^\[error\]" /home/neardws/openharmony-build/logs/hcodec/<run_id>/summary.txt
```

---

## 7. 看什么才算有效证据

### 7.1 surface 最小验证

重点看：

- `surface_minimal.log` 里是否出现：
  - `[phase] set output surface`
  - `[surface] ...`
  - `[summary] ... surface_cb=... surface_released=...`
- 判断口径：
  - `surface_cb > 0` 说明 consumer listener 至少被触发了
  - `surface_released > 0` 说明至少成功 `AcquireBuffer + ReleaseBuffer` 过

### 7.2 buffer sync / async 对比

重点看：

- `buffer_sync.log`
- `buffer_async.log`
- 两个 `[summary]` 里的：
  - `elapsed_ms`
  - `out_fps`
  - `output`
  - `out_cb`

判断口径：

- `async` 如果明显提高 `out_fps`，说明 output drain 确实是当前吞吐瓶颈之一
- 如果 `sync` 和 `async` 差距很小，瓶颈更可能在 feed、codec 内部或 surface/buffer 之外

---

## 8. 推荐上板顺序

仍然建议按这个顺序：

1. `surface` 先看“有没有真正出 surface buffer”
2. `buffer_sync` 建基线
3. `buffer_async` 看是否能抬吞吐

这样可以把“surface 能不能跑”和“buffer 能跑多快”分开。

---

## 9. 进展同步约定

这条链路后续只要有以下任一进展，都应该同步到本地仓库 `openharmony-rk3588-device-log`，再决定是否推到 GitHub：

- `surface` 首次拿到稳定 `surface_cb` / `surface_released`
- `buffer async` 明显优于 `buffer sync`
- 定位到 `63570434` / `0x3ca0202` 的明确错误码来源
- ABI / loader / runtime 限制发生变化
- 新的设备日志能证明“能跑”或“达不到 `real-time fps`”

> 注意：本地仓库更新是内部整理；真正 `git push` 到 GitHub 属于外部动作，按当前工作约定应在明确需要时再推。
