# 串口连接调试日志 2026-03-27

## 概述

本次会话完成了从串口连接到获取 shell、启用 HDC TCP 的全流程，并尝试通过串口传输 dropbear (SSH daemon)。

---

## 一、CP2102 接线问题排查

### 症状
- CP2102 识别为 COM3，但串口完全无数据

### 排查过程
1. 初次打开串口（1500000 波特率）收到大量乱码 → 波特率猜测不准确，或电气问题
2. 尝试各波特率扫描（115200 / 921600 / 1500000 / 3000000 / 460800）→ 全部乱码或无数据
3. 发现之前收到的回显是 TX/RX 短接（loopback）→ **TX 和 RX 接反了**

### 解决
- 将 CP2102 的 TX↔RX 对调后：
  - 1500000 波特率收到 **98.2% 可打印** 数据
  - 看到 `/system/bin/sh` 和 `#` 提示符
  - **确认正确波特率：1500000**

### 结论
- 板子 DEBUG 口引脚顺序需注意，RX/TX 容易接反
- CP2102 是 3.3V，板子 DEBUG 口也是 3.3V，电压匹配

---

## 二、串口 Shell 交互

### 环境
- 波特率：**1500000**
- 数据位：8，停止位：1，无校验
- Windows COM3

### Shell 特点
- 提示符：`#`（root）
- 存在大量 SELinux audit 日志混杂在输出中（permissive 模式）
- 部分命令受 SELinux 限制但仍可执行（permissive=1）

### FIQ Debugger 问题
- 串口同时用于 FIQ hardware debugger
- 长时间连接后会意外触发 FIQ debugger，进入 `debug>` 提示符
- **退出方式**：输入 `console` 命令返回普通 shell
- **问题**：`console` 切换后 shell 的 stdout 失效，命令执行但无输出
- **根本原因**：FIQ debugger `console` 切换后 shell 的 fd1/fd2 未正确绑定到串口

### 解决 stdout 失效
- 唯一可靠方法：物理重启板子
- 重启后第一次 shell 输出正常

---

## 三、HDC TCP 模式启用

### 目标
通过网络（而非 USB）连接 HDC（HarmonyOS Debug Connector）

### 步骤

#### 1. 设置 HDC 参数
```sh
param set persist.hdc.mode tcp
param set persist.hdc.port 5555
param set persist.hdc.authlevel 0
```

#### 2. 启动 HDC TCP
```sh
# 错误方式（此版本不支持）：
hdcd -s TCP:5555   # unrecognized option: s

# 正确方式（通过 killall 触发 init 重启 hdcd）：
killall hdcd
# init 自动重启 hdcd，读取 persist.hdc.mode=tcp 参数
```

#### 3. 确认监听
```
tcp  0.0.0.0:5555  LISTEN  hdcd
```

### HDC 客户端连接
```
hdc.exe tconn 192.168.31.56:5555
# 输出：Connect OK
hdc.exe list targets
# 输出：192.168.31.56:5555
```

### HDC Shell 成功
```
hdc.exe -t 192.168.31.56:5555 shell uname -a
# Linux localhost 5.10.110-geaba2291341c-dirty #1 SMP Thu Nov 27 17:47:23 CST 2025 aarch64
```

### HDC 认证问题（后续）
- HDC 连接在板子重启后失效
- 原因：RSA key 认证，每次重启后 hdcd 读取 hdc_keys 文件
- 尝试将客户端公钥写入 `/data/misc/hdc/hdc_keys`：
  ```sh
  mkdir -p /data/misc/hdc
  echo '<pubkey>' > /data/misc/hdc/hdc_keys
  chmod 600 /data/misc/hdc/hdc_keys
  ```
- 结果：仍然无法稳定连接（`[Fail]Not match target founded, check connect-key please`）
- **根本原因待查**：可能 TCP 模式的 key 验证机制与 USB 模式不同

### hdcd 命令参数（此版本）
```
hdcd -h
# -b  Daemon run in background/fork mode  （实际不支持）
# -u  Enable USB mode
# -t  Enable TCP mode
```

---

## 四、发现的服务

`netstat -tlnp` 输出的监听服务：

| 端口 | 服务 | 说明 |
|------|------|------|
| 1883 | mosquitto | MQTT broker |
| 27681 | ttyd | Web terminal (浏览器可直接访问) |
| 34687/40207/37745 | softbus_server | OpenHarmony 软总线 |
| 50061 | khcontainercomm | 容器通信 |
| 5555 | hdcd | HDC TCP (手动启动后) |

**ttyd Web Terminal**：直接访问 `http://192.168.31.56:27681` 即可获得浏览器 shell

---

## 五、通过串口传输 dropbear（SSH daemon）

### 背景
- 板子上没有 sshd / dropbear
- HDC TCP 连接不稳定
- 需要 SSH 以支持加密连接、文件传输、端口转发

### 编译 dropbear（aarch64 静态）

在 x86 Linux 上交叉编译：

```sh
# 安装交叉编译器
sudo apt-get install -y gcc-aarch64-linux-gnu

# 下载源码
wget https://matt.ucc.asn.au/dropbear/releases/dropbear-2024.86.tar.bz2
tar -xjf dropbear-2024.86.tar.bz2
cd dropbear-2024.86

# 禁用密码认证（避免 crypt() 依赖）
echo '#define DROPBEAR_SVR_PASSWORD_AUTH 0' >> localoptions.h
echo '#define DROPBEAR_CLI_PASSWORD_AUTH 0' >> localoptions.h

# 交叉编译静态二进制
./configure --host=aarch64-linux-gnu --enable-static --disable-zlib --disable-pam
make PROGRAMS="dropbear dropbearkey dbclient" STATIC=1

# 结果
aarch64-linux-gnu-strip dropbear
gzip -9 dropbear   # 1.3MB → 524KB
```

### 传输方案：串口 base64

由于 HDC 不稳定，使用串口逐行 echo base64 数据的方案：

```
base64 dropbear.gz > dropbear.b64   # 9404 行
```

PowerShell 脚本逐行发送（每行 80ms，约 12 分钟）：
```powershell
foreach ($line in $lines) {
    $port.Write("echo '$line' >> /data/local/db.b64`r`n")
    Start-Sleep -Milliseconds 80
}
# 然后解码
$port.Write("base64 -d /data/local/db.b64 > /data/local/db.gz`r`n")
$port.Write("gzip -d /data/local/db.gz`r`n")
$port.Write("chmod 755 /data/local/dropbear`r`n")
```

### 传输问题

1. **第一次**：写入 `/data/local/tmp/` → 板子重启后丢失（tmpfs）
2. **第二次**：改写入 `/data/local/`（f2fs 持久分区）→ SSH 连接在 8100 行断开
3. **SSH 断开原因**：长时间连接被服务器超时断开

### 关于 /data 分区
```
/dev/block/mmcblk0p12 on /data type f2fs (rw,...)
```
- `/data/local/` → **持久**（f2fs，重启保留）
- `/data/local/tmp/` → **tmpfs**（重启清空）

### 未完成
- 传输仍未完成（SSH 连接超时导致中断）
- 需要在不依赖 SSH 长连接的方式完成传输
- 考虑方案：在 Windows 本地直接运行 PowerShell 脚本（不经 SSH）

---

## 六、待解决问题

1. **dropbear 传输**：完成 aarch64 静态 dropbear 的串口传输并验证
2. **HDC TCP 认证**：排查 TCP 模式下 key 认证失败原因
3. **FIQ Debugger 触发**：找到避免意外触发 FIQ debugger 的方法（可能是特定字符序列触发）
4. **SSH 服务启动**：dropbear 装好后：
   ```sh
   # 生成 host key
   /data/local/dropbearkey -t rsa -f /data/local/dropbear_rsa_host_key
   # 启动
   /data/local/dropbear -r /data/local/dropbear_rsa_host_key -p 22 -F -E
   ```

---

## 七、关键信息汇总

| 项目 | 值 |
|------|-----|
| 板子 IP | 192.168.31.56 |
| 串口 | COM3，1500000 baud，8N1 |
| HDC TCP 端口 | 5555 |
| ttyd Web Terminal | http://192.168.31.56:27681 |
| hdc.exe 路径 | C:\Users\neardws\Desktop\hdc\hdc.exe |
| dropbear 编译好的位置 | Linux server: /tmp/dropbear-2024.86/dropbear |
| dropbear gz+b64 | Linux server: /tmp/dropbear.b64 (9404 行) |
| 目标写入路径 | /data/local/dropbear (持久分区) |
