# 宇视 IPC LightAPI v3.0 控制文档

摄像头 IP：`192.168.1.13`  
认证方式：**HTTP Digest Authentication**  
API 前缀：`http://192.168.1.13/LAPI/V1.0`

> 板子上已部署静态 curl：`/data/local/tmp/curl`（aarch64 musl 静态编译，无依赖）

---

## 设备信息

```bash
/data/local/tmp/curl -s --digest -u admin:'a1234567~' \
  'http://192.168.1.13/LAPI/V1.0/System/DeviceInfo'
```

实测结果：
| 字段 | 值 |
|------|-----|
| 型号 | IPC-B2A4-FW |
| 序列号 | 210235CBE03261000035 |
| 固件版本 | GIPC-B6220.6.5.251212 |
| 硬件版本 | A |

---

## 视频流参数

### 查询当前配置

```bash
/data/local/tmp/curl -s --digest -u admin:'a1234567~' \
  'http://192.168.1.13/LAPI/V1.0/Channels/0/Media/Video/Streams/DetailInfos'
```

当前三路流配置（2026-03-31 实测）：

| 码流ID | 分辨率 | 帧率 | 码率 | 编码 |
|--------|--------|------|------|------|
| 0（主码流） | 2688×1520 | 30fps | 16384 Kbps（16Mbps） | H.264 CBR |
| 1（子码流） | 720×576 | 30fps | 1024 Kbps | H.264 CBR |
| 2（第三码流）| 352×288 | 30fps | 128 Kbps | H.264 VBR |

### 修改帧率（以主码流为例，改为 25fps）

```bash
/data/local/tmp/curl -s --digest -u admin:'a1234567~' \
  -X PUT 'http://192.168.1.13/LAPI/V1.0/Channels/0/Media/Video/Streams/0' \
  -H 'Content-Type: application/json' \
  -d '{"VideoEncodeInfo":{"FrameRate":25}}'
```

### 修改码率（主码流改为 8192 Kbps = 8Mbps）

```bash
/data/local/tmp/curl -s --digest -u admin:'a1234567~' \
  -X PUT 'http://192.168.1.13/LAPI/V1.0/Channels/0/Media/Video/Streams/0' \
  -H 'Content-Type: application/json' \
  -d '{"VideoEncodeInfo":{"BitRate":8192}}'
```

---

## 抓图

```bash
# 抓取当前帧，保存到板子本地
/data/local/tmp/curl -s --digest -u admin:'a1234567~' \
  'http://192.168.1.13/LAPI/V1.0/Channels/0/Media/Video/Streams/0/Snapshot' \
  -o /data/local/tmp/snapshot.jpg
```

---

## 图像参数调整

### 查询增强参数（亮度/对比度/锐度/降噪）

```bash
/data/local/tmp/curl -s --digest -u admin:'a1234567~' \
  'http://192.168.1.13/LAPI/V1.0/Channels/0/Image/Enhance'
```

### 修改图像参数示例

```bash
/data/local/tmp/curl -s --digest -u admin:'a1234567~' \
  -X PUT 'http://192.168.1.13/LAPI/V1.0/Channels/0/Image/Enhance' \
  -H 'Content-Type: application/json' \
  -d '{"Brightness":50,"Contrast":50,"Saturation":50,"Sharpness":50}'
```

### 曝光设置

```bash
/data/local/tmp/curl -s --digest -u admin:'a1234567~' \
  'http://192.168.1.13/LAPI/V1.0/Channels/0/Image/Advanced/Exposure'
```

---

## 红外/补光灯控制

### 查询当前灯光状态

```bash
/data/local/tmp/curl -s --digest -u admin:'a1234567~' \
  'http://192.168.1.13/LAPI/V1.0/Channels/0/Image/LampCtrl'
```

实测返回（当前状态）：
```json
{
  "Enabled": 1,
  "Type": 6,
  "Mode": 0,
  "NearLevel": 0
}
```

### 灯光类型说明（从 Capabilities 获取）

| LampType | 说明 |
|----------|------|
| 1 | 红外灯（IR） |
| 2 | 白光灯 |
| 6 | 激光/其他 |

### 控制模式（Mode）

| Mode 值 | 含义 |
|---------|------|
| 0 | 自动（根据环境亮度自动切换） |
| 1 | 常开 |
| 3 | 常关 |

### 开启红外灯（强制常开）

```bash
/data/local/tmp/curl -s --digest -u admin:'a1234567~' \
  -X PUT 'http://192.168.1.13/LAPI/V1.0/Channels/0/Image/LampCtrl' \
  -H 'Content-Type: application/json' \
  -d '{"Enabled":1,"Type":1,"Mode":1,"NearLevel":100}'
```

### 关闭红外灯

```bash
/data/local/tmp/curl -s --digest -u admin:'a1234567~' \
  -X PUT 'http://192.168.1.13/LAPI/V1.0/Channels/0/Image/LampCtrl' \
  -H 'Content-Type: application/json' \
  -d '{"Enabled":1,"Type":1,"Mode":3}'
```

### 恢复自动模式

```bash
/data/local/tmp/curl -s --digest -u admin:'a1234567~' \
  -X PUT 'http://192.168.1.13/LAPI/V1.0/Channels/0/Image/LampCtrl' \
  -H 'Content-Type: application/json' \
  -d '{"Enabled":1,"Type":1,"Mode":0}'
```

### 昼夜模式（日夜切换）

该摄像头支持 `DayNight` 控制（从 Capabilities 确认）：
- Mode 0：自动
- Mode 1：彩色（日间）
- Mode 2：黑白（夜间/红外）
- Mode 3：定时切换

```bash
# 强制切换为夜间黑白模式
/data/local/tmp/curl -s --digest -u admin:'a1234567~' \
  -X PUT 'http://192.168.1.13/LAPI/V1.0/Channels/0/Image/Advanced/Exposure' \
  -H 'Content-Type: application/json' \
  -d '{"DayNight":{"Mode":2}}'
```

---

## 其他支持的图像能力（从 Capabilities 确认）

| 功能 | 支持 |
|------|------|
| 锐度调节 | ✅ |
| 2D 降噪 | ✅ |
| 3D 降噪 | ✅ |
| 宽动态（WDR） | ✅（3档：关/自动/手动）|
| 曝光补偿 | ✅ |
| 测光模式 | ✅（4种）|
| 快门时间调节 | ✅（27档）|
| 白平衡 | ✅（7种模式）|
| 日夜切换 | ✅（4种模式）|
| EPTZ | ❌ |
| 自动对焦 | ❌ |

---

## 注意事项

- eth1 IP 重启后会丢失，每次需重新配置：`ifconfig eth1 192.168.1.100 netmask 255.255.255.0 up`
- PUT 请求只需传要修改的字段，不需要传完整 JSON
- `ResponseCode: 0` 表示成功，`ResponseCode: 2` 表示参数错误

---

*记录时间：2026-03-31*
