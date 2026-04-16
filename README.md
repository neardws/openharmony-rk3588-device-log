# OpenHarmony RK3588 Device Log

Private notes and logs for diagnosing and documenting a custom RK3588/OpenHarmony device, plus a recent media/codec validation track that moved onto a new RK3568 board.

## Current Summary

- Exact board model confirmed by user: `DC-A588-V04`
- Historical baseline device family in this repo: industrial/custom RK3588 board
- Recent media/codec validation board (2026-04-14 to 2026-04-16): `rockchip,rk3568-toybrick-dev-linux-x0`
- Current OS observed: ZilOS (reported as based on OpenHarmony 4.1.3)
- Known working path: device gets DHCP lease from router
- Known DHCP leases observed on different Ethernet connections: `192.168.31.176`, `192.168.31.196`
- Likely debug entry points:
  - 4-pin `DEBUG` header (TTL serial console)
  - `BOOT` button (possible Rockchip loader/maskrom/boot mode entry)
  - TF card slot (possible storage expansion / upgrade path)

## Repo Split Note (2026-04-16)

The repo is now tracking two different boards:

- Older content in this repo, especially earlier hardware / NPU / build / Ubuntu notes, is still about the original **RK3588** board.
- Only the last 2 to 3 days of newly added OpenHarmony media / codec / benchmark content is on the new **RK3568** board.

Recent live checks on the new board show:

- `/proc/device-tree/compatible`: `rockchip,rk3568-toybrick-dev-linux-x0`
- `/sys/devices/system/cpu/online`: `0-3`
- `/proc/cpuinfo`: only four `CPU part : 0xd05` cores are exposed

That combination matches an RK3568-class board. It should be used to label the recent media/codec validation docs, but it should **not** be used to retroactively relabel the older RK3588 material in this repo.


## Confirmed from official PDF

- Official board model confirmed: `DC-A588-V04`
- Official manual confirms the 4-pin DEBUG header is the default system debug UART: `ttys2`
- Official manual confirms the board has **2 independent gigabit Ethernet ports** (`网口0` / `网口1`)
- Official manual confirms the dual stacked Type-A USB includes:
  - upper port = `USB 2.0 OTG`
  - lower port = `USB 3.0 Host`
- Official manual confirms upgrade paths via **PC / U-disk / TF card**
- Official manual confirms the `BOOT` key is relevant to Loader / Maskrom style recovery / flashing entry

## Goals

1. Identify exact board / boot chain / storage layout
2. Capture serial logs from DEBUG port
3. Determine viable recovery / flashing paths
4. Document interface map, quirks, and troubleshooting steps

## Directory Layout

- `hardware/` — board interface map, photos, labels, notes
- `network/` — IP info, connectivity tests, port scans
- `debug/` — USB/ADB attempts, serial settings, console notes
- `flashing/` — boot modes, upgrade paths, recovery notes
- `issues/` — tracked problems and hypotheses
- `logs/` — raw logs and command outputs
- `build/` — board config, build plans, build logs, reusable patch notes

## Recent Build Docs

- `build/docs/rk3568-full-build-fixset-2026-04-11.md` — reusable fix set for `rk3568@hihope` + `linux-5.10`
- `build/docs/rk3568-full-build-patch-checklist-2026-04-11.md` — short patch checklist for reproducing the successful build

## Current Findings

### Interfaces observed

- Basic:
  - 2x FAN
  - 2x SATA (one with nearby power)
  - 3x UART
  - 1x RS485
  - 1x CAN
- USB / control:
  - 4x 4-pin internal USB headers
  - 1x KEY
  - 1x TP
  - 1x GPIO
  - 2x BL
  - 1x LVDS
- Display / audio:
  - 2x EDP + 1x 2-pin EDP-related header
  - 1x SPK
  - 3x HDMI
- Network / external:
  - 2x ETH
  - 2x USB-A
  - 1x TF slot
  - 1x SIM slot
- Power / buttons:
  - Buttons: `PWR`, `UP`, `BOOT`
  - 12V DC-IN
  - Phone/audio jack

### Network

- Direct Ethernet to laptop gave self-assigned IP on laptop side
- When connected to router, device received DHCP address `192.168.31.176`
- ICMP ping to device works
- Common remote ports tested closed/refused: 22, 23, 80, 443, 5555, 8080, 8710
- TCP port 53 was observed open

### USB debugging attempts

- Developer options showed only `USB 调试`
- Enabling USB debugging did not expose a visible adb device through tested USB-A path
- Strong hypothesis: tested USB-A ports are host-only, not OTG/device-mode debug port

### Working hypotheses

1. The 4-pin `DEBUG` header is the primary serial console
2. `BOOT` may enter Rockchip loader / recovery mode
3. TF slot may support upgrade or alternate boot in some scenarios
4. A true OTG/device-mode port may be absent, hidden, internal, or not yet identified

## Serial Console — Confirmed Working (2026-03-27)

- **波特率**: 1500000 (8N1)
- **COM 口**: CP2102 → Windows COM3
- **接线**: CP2102 RXD → 板子 TX，CP2102 TXD → 板子 RX，GND → GND
- **Shell**: root (`#`)，OpenHarmony 4.x，aarch64
- **内核**: Linux 5.10.110 aarch64
- **板子 IP**: 192.168.31.56（DHCP）
- **注意**: TX/RX 容易接反，需对调确认

## HDC TCP — 已启用

```sh
param set persist.hdc.mode tcp
param set persist.hdc.port 5555
param set persist.hdc.authlevel 0
killall hdcd   # init 自动重启并读取 TCP 参数
```

连接：`hdc.exe tconn 192.168.31.56:5555`

## 可用网络服务

| 端口 | 服务 |
|------|------|
| 27681 | ttyd Web Terminal（浏览器访问）|
| 1883 | MQTT (mosquitto) |
| 5555 | HDC TCP（手动启用）|

## Next Actions

- [ ] 完成 dropbear 串口传输（目标: `/data/local/dropbear`）
- [ ] 启动 SSH 服务（dropbear）
- [ ] 排查 HDC TCP 认证问题（重启后 key 失效）
- [ ] 找到避免触发 FIQ Debugger 的方法
- [ ] 记录 U-Boot 启动日志
