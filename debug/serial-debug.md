# 串口调试笔记

## 已确认配置

- 调试口：`ttys2`（板上 4Pin 白色插座）
- 引脚：`VCC / RX2 / TX2 / GND`（VCC 不接）
- **正确接线**：CP2102 RXD → 板子 TX2，CP2102 TXD → 板子 RX2，GND → GND
- **波特率：1500000**（8N1）—— 已确认，其他波特率无效

> ⚠️ TX/RX 容易接反！接线后如果全是乱码或 0 字节，先尝试对调 TX↔RX。

## 调试工具（Windows）

使用 PowerShell + .NET SerialPort 通过 SSH 远程控制：

```powershell
$port = new-object System.IO.Ports.SerialPort 'COM3',1500000,'None',8,'One'
$port.ReadTimeout = 1000
$port.Open()
$port.Write("command`r`n")
Start-Sleep -Milliseconds 1500
$r = $port.ReadExisting()
# 过滤不可打印字符
$c = ($r.ToCharArray() | ForEach-Object { 
    if ([int]$_ -ge 32 -and [int]$_ -le 126 -or [int]$_ -eq 10) { $_ } else { '.' }
}) -join ''
Write-Host $c
$port.Close()
```

## FIQ Debugger 问题

串口同时用于硬件 FIQ debugger。长时间连接或特定字符序列会触发，进入 `debug>` 提示符。

**FIQ Debugger 有用命令：**
```
console    # 返回普通 shell（但 stdout 会失效）
ps         # 进程列表
kmsg       # 内核日志
reboot     # 重启
help       # 帮助
```

**stdout 失效问题**：
- `console` 切回后 shell 的 fd1/fd2 未绑定到串口
- 命令会执行但无输出
- 唯一可靠修复方法：物理重启板子

## Shell 环境

- 用户：root（`#` 提示符）
- SELinux：permissive 模式（有大量 audit 日志但命令仍执行）
- 内核：Linux 5.10.110-geaba2291341c-dirty aarch64
- OS：OpenHarmony（ZilOS 定制版）

## 分区信息

```
/dev/block/mmcblk0p6   /         ext4  (ro)
/dev/block/mmcblk0p7   /vendor   ext4  (ro)
/dev/block/mmcblk0p12  /data     f2fs  (rw) ← 持久可写分区
```

**重要**：`/data/local/tmp/` 是 tmpfs，重启清空。写文件应用 `/data/local/`。

## 串口传文件（base64 方案）

```bash
# 在 Linux 上
gzip -9 binary_file
base64 file.gz > file.b64   # 约 9400 行/524KB

# PowerShell 逐行写入
foreach ($line in (Get-Content file.b64)) {
    $port.Write("echo '$line' >> /data/local/file.b64`r`n")
    Start-Sleep -Milliseconds 80
}
# 解码
$port.Write("base64 -d /data/local/file.b64 > /data/local/file.gz`r`n")
$port.Write("gzip -d /data/local/file.gz`r`n")
```

**注意**：SSH 长连接容易超时断开，建议直接在 Windows 本地运行 PowerShell 脚本。

## 详细调试日志

见 [serial-connection-log-2026-03-27.md](serial-connection-log-2026-03-27.md)
