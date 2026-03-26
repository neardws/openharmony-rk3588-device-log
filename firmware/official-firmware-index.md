# 官方固件索引（DC_A588 / A588）

> 来源：定昌官网产品页导出 PDF + 用户整理的文本版下载清单。
> 目标：把 A588 最相关的官方固件按系统分类整理，便于后续选型、对照与排错。

## 使用前的关键判断

A588 官方固件明显按以下维度分化：

- 板型：`A588` / `C588` / `CM588`
- 显示配置：`LVDS1920x1080` / `LVDS1280x800` / `LVDS800x1280` / `1920x1200` / `1024x768`
- 触控配置：`gt9xx`
- 摄像头/视频链路：`csi` / `AHD`
- 是否带 QT：`qtcom`

**这说明不能把“同一系统版本”视为单一镜像。**
刷错分辨率、触控版本或板型，极有可能导致显示/触控/摄像头/外设异常。

---

## 一、OpenHarmony / 鸿蒙 4.1.3（A588 最相关）

### A588

1. `GB-A588-OHOS4.1.3-LVDS1920x1080n-main-20250722-1552.rar`
   - 下载：<https://pan.baidu.com/s/1ubU43JtvTxSmsVa-mJxiYg?pwd=1234>
   - 板型：A588
   - 日期：2025-07-22
   - 备注：`LVDS1920x1080` + `main`

2. `GB-A588-OHOS4.1.3-LVDS1920x1080n-gt9xx-20250506-1648.rar`
   - 下载：<https://pan.baidu.com/s/1jnfsyxpJUpy3ixHCI03lAg?pwd=1234>
   - 板型：A588
   - 日期：2025-05-06
   - 备注：`LVDS1920x1080` + `gt9xx`

3. `GB-A588-OHOS4.1.3-LVDS1280x800-gt9xx-20250809-1104.rar`
   - 下载：<https://pan.baidu.com/s/1Pkg9vUSO60yRQY7pcOwLQg?pwd=1234>
   - 板型：A588
   - 日期：2025-08-09
   - 备注：`LVDS1280x800` + `gt9xx`

4. `GB-A588-OHOS4.1-LVDS800x1280-gt9xx-20250505-1927.rar`
   - 下载：<https://pan.baidu.com/s/109ZDBDTdOes_ntSn4nFc-A?pwd=1234>
   - 板型：A588
   - 日期：2025-05-05
   - 备注：`LVDS800x1280` + `gt9xx`

### CM588

5. `GB-CM588-OHOS4.1.3-20250708-1709.rar`
   - 下载：<https://pan.baidu.com/s/1ZfwOY7IrzOrTILrUmRU5HA?pwd=1234>
   - 板型：CM588
   - 日期：2025-07-08

### 当前对排查最有用的结论

- A588 的鸿蒙固件至少明确分了：
  - `1920x1080 main`
  - `1920x1080 gt9xx`
  - `1280x800 gt9xx`
  - `800x1280 gt9xx`
- 这说明 **显示分辨率 + 触控芯片** 是官方明确区分的刷机维度。
- 如果当前设备是 LVDS 屏，后续必须先确认：
  - 屏幕分辨率
  - 是否为 GT9xx 触控

---

## 二、星光麒麟（A588）

1. `GB-A588-starkylin-250726.023606-csi.rar`
   - 下载：<https://pan.baidu.com/s/114EE1wBkOm_bzirIDo4nRA?pwd=1234>
   - 板型：A588
   - 日期：2024-09-19（来源文本记录）
   - 备注：`csi`

2. `GB-A588-starkylin-250729.113611-AHD.rar`
   - 下载：<https://pan.baidu.com/s/1dX29WpF0R2OLc6x9-pLfsg?pwd=1234>
   - 板型：A588
   - 日期：2025-07-30
   - 备注：`AHD`

3. `GB-A588-starkylinV1.0-250427-185819-LVDS-1920x1080.rar`
   - 下载：<https://pan.baidu.com/s/124qOD4zTw1bUtPL0Seb_IQ?pwd=dcdz>
   - 提取码：`dcdz`
   - 板型：A588
   - 日期：2025-04-28

4. `GB-A588-starkylinV1.0-250710-161619-LVDS-1920x1080.img`
   - 下载：<https://pan.baidu.com/s/1Qsy9gnH0OcXhHKfGxlH1zw?pwd=dcdz>
   - 提取码：`dcdz`
   - 板型：A588
   - 日期：2025-07-21

5. `GB-A588-starkylinV1.0-250819-153245.img`
   - 下载：<https://pan.baidu.com/s/1Q3lLcPo5kK0zy1AKe8kWOQ?pwd=dcdz>
   - 提取码：`dcdz`
   - 板型：A588
   - 日期：2025-08-22

### 结论

- 星光麒麟固件同样按 `CSI / AHD / LVDS1920x1080` 分化。
- 这进一步说明 A588 官方固件体系不是“一个镜像适配所有外设”。

---

## 三、银河麒麟（A588）

### 早期 A588 包

1. `GB_A588_Kylin_qtcom_20221221-082825_1280x800_v01.zip`
   - 下载：<https://pan.baidu.com/s/1TLvBGzk93gc1VC_WS63PVw?pwd=iqqe>
   - 提取码：`iqqe`
   - 板型：A588
   - 日期：2022-12-21

2. `GB_A588_Kylin_qtcom_20221221-081733_1920x1200_v01.zip`
   - 下载：<https://pan.baidu.com/s/104Jw-fw260t-KjULO3VQUQ?pwd=3n85>
   - 提取码：`3n85`
   - 板型：A588
   - 日期：2022-12-21

3. `GB_A588v03_kylin_20230316-065931_1024x768_v01.zip`
   - 下载：<https://pan.baidu.com/s/1AfiIuIyZHdyMkKrYDqJa1Q?pwd=8ya5>
   - 提取码：`8ya5`
   - 板型：A588
   - 日期：2023-03-16

4. `GB_A588v03_kylin_20230413-104852_1024x768_v02.zip`
   - 下载：<https://pan.baidu.com/s/1s5rCSw9XZuqY1RoE4Z5NyQ?pwd=98wh>
   - 提取码：`98wh`
   - 板型：A588
   - 日期：2023-04-13

5. `GB_A588v03_kylin_20230424-104627_1920x1080_v04.zip`
   - 下载：<https://pan.baidu.com/s/1mg14lttbgeXiIgLesB6uHg?pwd=rjv3>
   - 提取码：`rjv3`
   - 板型：A588
   - 日期：2023-04-24

6. `GB_RK3588_Kylin_20231111-171245_v2.1.zip`
   - 下载：<https://pan.baidu.com/s/1_y8zqZ1AGNP8U5hvqSarhw?pwd=1234>
   - 提取码：`1234`
   - 板型：A588
   - 日期：2023-11-12

7. `GB_A588_Kylin_20240319-102952_1920X1080_v0.1.rar`
   - 下载：<https://pan.baidu.com/s/1KVG1nFOk1vteQ_gJaHQbgg?pwd=wqug>
   - 提取码：`wqug`
   - 板型：A588
   - 日期：2024-03-19

### Kylin V10（A588）

8. `GB_A588_KylinV10_20240629_183817_V1.55.rar`
   - 下载：<https://pan.baidu.com/s/1uu35remDrS5KMcrzrdMBNg?pwd=dcdz>
   - 提取码：`dcdz`
   - 板型：A588
   - 日期：2024-06-29

9. `GB_A588_KylinV10_20250419_151908_1920x1080_V1.59.img`
   - 下载：<https://pan.baidu.com/s/1Jmm0fziZ62dkaI_6SXxQhw?pwd=dcdz>
   - 提取码：`dcdz`
   - 板型：A588
   - 日期：2025-04-19

10. `GB-A588-KylinV10-2403-20250809-164725-1920x1080-V1.6.img`
    - 下载：<https://pan.baidu.com/s/1U0NS5tfVEt7u3lTFJV7u_A?pwd=dcdz>
    - 提取码：`dcdz`
    - 板型：A588
    - 日期：2025-08-09

### 银河麒麟带 QT（A588）

11. `GB_A588_Kylin_20240411-115935_lvds1920x1080_v0.2.rar`
    - 下载：<https://pan.baidu.com/s/1DADlAWIDmdmqxFjs1y-DmQ>
    - 提取码：`it1s`
    - 板型：A588
    - 日期：2024-04-11

12. `GB_A588_KylinV10_20241127_142638_qtcom_V1.6.img`
    - 下载：<https://pan.baidu.com/s/1Snjt3nm_pPL1nd6KtPZc7g?pwd=pbkh>
    - 提取码：`pbkh`
    - 板型：A588
    - 日期：2024-11-27

### 结论

- 银河麒麟固件历史最长，覆盖 2022 → 2025。
- 明显围绕显示配置反复迭代：
  - `1280x800`
  - `1920x1200`
  - `1024x768`
  - `1920x1080`
- 这再次支持“显示配置是板级适配关键变量”。

---

## 四、跨板型共用包 / 对照项

### A588-C588 都支持

- `GB_C588_Kylin_qtcom_20230216-013301_dualHdmi_v02.zip`
  - 下载：<https://pan.baidu.com/s/1jBPTYZIhFBjsReHx1gPp1w?pwd=bc5y>
  - 提取码：`bc5y`
  - 板型：A588-C588 都支持
  - 日期：2023-02-16

- `GB_RK3588_Kylin_20231201-140704_gpu_v2.2.zip`
  - 下载：<https://pan.baidu.com/s/16BHJt877bwdHzO5me7onPQ>
  - 提取码：`1234`
  - 板型：A588-C588 都支持
  - 日期：2023-12-01

### 对当前设备的意义

- A588 与 C588 / CM588 很可能共享一部分核心板级逻辑，但底板/显示/外设布局不同。
- 可以用于横向参考，但**不能直接假设镜像完全互刷兼容**。

---

## 五、当前设备最值得关注的固件选择线索

结合我们当前设备状态，后续最该优先确认的是：

1. **屏幕是不是 LVDS**
2. **屏幕分辨率到底是多少**
   - 1920x1080
   - 1280x800
   - 800x1280
   - 其他
3. **触控是不是 GT9xx**
4. **摄像头方案是不是 AHD / CSI**
5. **当前 ZilOS / OHOS 4.1.3 更接近哪一个官方包**

---

## 六、目前对排查最有帮助的候选包（仅作定位，不建议盲刷）

### 如果当前是 LVDS 1920x1080
优先参考：
- `GB-A588-OHOS4.1.3-LVDS1920x1080n-main-20250722-1552.rar`
- `GB-A588-OHOS4.1.3-LVDS1920x1080n-gt9xx-20250506-1648.rar`

### 如果当前是 LVDS 1280x800
优先参考：
- `GB-A588-OHOS4.1.3-LVDS1280x800-gt9xx-20250809-1104.rar`

### 如果当前是竖屏 800x1280
优先参考：
- `GB-A588-OHOS4.1-LVDS800x1280-gt9xx-20250505-1927.rar`

---

## 七、操作建议

### 现在
- 继续记录当前设备的：
  - 双网口行为
  - 显示分辨率/输出方式
  - 触控型号线索
  - 当前系统版本与 UI 特征

### 等 USB-TTL 到货
- 先抓启动日志
- 确认：
  - panel / lvds / touch / gmac / wifi 初始化信息
  - 板载 dtb / bootargs / 分区
- 再决定是否需要对照官方 OHOS 4.1.3 包

### 刷机前
- 先备份
- 先确认分辨率 / 触控 / 具体板型
- 不要直接拿 A588 / C588 / CM588 互刷

