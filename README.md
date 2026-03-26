# OpenHarmony RK3588 Device Log

Private notes and logs for diagnosing and documenting a custom RK3588/OpenHarmony device.

## Current Summary

- Device family: likely industrial/custom RK3588/RK3588S board
- Current OS observed: ZilOS (reported as based on OpenHarmony 4.1.3)
- Known working path: device gets DHCP lease from router
- Known device IP (observed once): `192.168.31.176`
- Likely debug entry points:
  - 4-pin `DEBUG` header (TTL serial console)
  - `BOOT` button (possible Rockchip loader/maskrom/boot mode entry)
  - TF card slot (possible storage expansion / upgrade path)

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

## Next Actions

- [ ] Buy USB-TTL adapter (prefer CP2102)
- [ ] Connect to DEBUG header (GND/TX/RX only; do not connect VCC)
- [ ] Test serial baud rates: 1500000, 115200, 921600
- [ ] Capture boot log
- [ ] Identify U-Boot / kernel / storage / recovery clues
