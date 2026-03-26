# 串口调试计划（基于正式规格书）

## 已确认

- 正式调试口：`ttys2`
- 物理接口：4Pin 白色插座
- 引脚：`VCC / RX2 / TX2 / GND`
- 用途：默认系统调试口，上电应输出启动日志

## 接线

- USB-TTL `RX` → 板子 `TX2`
- USB-TTL `TX` → 板子 `RX2`
- USB-TTL `GND` → 板子 `GND`
- 不接 `VCC`

## 串口参数优先级

1. `1500000`
2. `115200`
3. `921600`

## 首次上电需要关注的信息

- BootROM / miniloader 输出
- 是否出现 `U-Boot`
- 是否出现 `Hit any key to stop autoboot`
- eMMC / TF / SATA 检测信息
- 网口驱动 / PHY 初始化信息
- Linux kernel log
- 是否落到登录提示符
