# Camera — RTSP 接入与解码

## 硬件连接

- 摄像头型号：宇视 IPC
- 连接方式：网线直连至开发板 **eth1** 口
- 摄像头 IP：`192.168.1.13`
- 板端 eth1 配置：`192.168.1.100/24`

### 配置命令（重启后需重新设置）

```bash
ifconfig eth1 192.168.1.100 netmask 255.255.255.0 up
```

### 连通性验证

```bash
ping -c 3 192.168.1.13
# 预期：0% 丢包，延迟 ~1ms
```

---

## RTSP 流信息

| 码流 | URL |
|------|-----|
| 主码流（高清） | `rtsp://admin:a1234567~@192.168.1.13:554/media/video1` |
| 子码流（标清） | `rtsp://admin:a1234567~@192.168.1.13:554/media/video2` |

- 编码格式：H.264 Main Profile
- 分辨率：2688 × 1520
- 帧率：12 fps
- 音频：PCM mulaw，8000Hz，mono

---

## FFmpeg 部署

由于 OpenHarmony 使用 musl libc，需使用完全静态编译的 FFmpeg 二进制。

### 下载来源

```
https://johnvansickle.com/ffmpeg/releases/ffmpeg-release-arm64-static.tar.xz
```

版本：FFmpeg 7.0.2，aarch64，statically linked（无外部依赖）

### 部署步骤

```bash
# 在 Ubuntu 上下载并解压
wget https://johnvansickle.com/ffmpeg/releases/ffmpeg-release-arm64-static.tar.xz
tar -xf ffmpeg-release-arm64-static.tar.xz

# 传到 Windows（通过 SCP）
scp ffmpeg-7.0.2-arm64-static/ffmpeg neardws@<windows-ip>:C:/Users/neardws/Desktop/ffmpeg

# 从 Windows 用 HDC 推到板子
hdc.exe file send C:\Users\neardws\Desktop\ffmpeg /data/local/tmp/ffmpeg
hdc.exe shell "chmod +x /data/local/tmp/ffmpeg"

# 验证
hdc.exe shell "/data/local/tmp/ffmpeg -version"
```

---

## 解码验证

### 验证 RTSP 连通与解码能力（跑 3 秒）

```bash
hdc.exe shell "/data/local/tmp/ffmpeg -rtsp_transport tcp \
  -i 'rtsp://admin:a1234567~@192.168.1.13:554/media/video1' \
  -t 3 -f null -"
```

预期输出：`frame=36 fps=25 speed=2.08x`（CPU 软解，实时无压力）

### 抓取单帧 RGB24 原始数据

```bash
hdc.exe shell "/data/local/tmp/ffmpeg -rtsp_transport tcp \
  -i 'rtsp://admin:a1234567~@192.168.1.13:554/media/video1' \
  -vframes 1 -f rawvideo -pix_fmt rgb24 /data/local/tmp/frame.rgb"
```

输出文件：`/data/local/tmp/frame.rgb`，大小约 12MB（2688×1520×3 bytes）

### 将 RGB raw 转为 PNG 预览（在 Ubuntu 上）

```bash
# 从板子拉回 Windows
hdc.exe file recv /data/local/tmp/frame.rgb C:\Users\neardws\Desktop\frame.rgb

# 从 Windows SCP 到 Ubuntu
scp neardws@<windows-ip>:C:/Users/neardws/Desktop/frame.rgb /tmp/frame.rgb

# Python 转 PNG
python3 -c "
from PIL import Image
data = open('/tmp/frame.rgb', 'rb').read()
img = Image.frombytes('RGB', (2688, 1520), data)
img.save('/tmp/frame.png')
"
```

---

## 当前状态与后续计划

| 项目 | 状态 |
|------|------|
| 摄像头网络连通 | ✅ |
| RTSP 拉流 | ✅ |
| FFmpeg 软解（CPU） | ✅ |
| RGB24 帧输出 | ✅ |
| MPP 硬件解码（rkmpp） | ⏳ 待实现 |
| NPU 推理接入 | ⏳ 暂搁置，待 Leader 确认 |

### MPP 硬解升级路径

板子上已有 `/vendor/lib64/librockchip_mpp.z.so`，后续可：
1. 交叉编译带 `--enable-rkmpp` 的 FFmpeg（需 OHOS SDK 工具链）
2. 或直接用 MPP C API 写解码程序，绕过 FFmpeg

---

*记录时间：2026-03-31*
