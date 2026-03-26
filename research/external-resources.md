# 外部资料整理：DC-A588 / DC-A588D5-V01

> 目标：收集 GitHub / 社区 / 网络上与当前板子相关的公开资料，便于后续对照调试、刷机与适配。

## 一、最有价值的外部资料

### 1. GitHub 资料归档仓库

**AndroidOL/Archive-DC-A588**  
<https://github.com/AndroidOL/Archive-DC-A588>

#### 已确认内容

- 这是目前最关键的公开资料仓库之一
- 包含：
  - 板子规格书 PDF
  - 官方镜像解包内容
  - 官方镜像提取的 dtb / dts
  - 板上提取的 dts
  - 第三方镜像资料

#### README 中明确提到

- 官方 Ubuntu 镜像文件名：
  - `A588_ubuntu20.04_20230202-135921_V1.0.img`
- Loader 路径：
  - `image-unpack/Android/MiniLoaderAll.bin`
- README 强调刷机时：
  - **进入 LOADER 模式，不是 MASKROM 模式**

#### 仓库顶层可见内容

- `DC-A588-V04规格说明书2024-10-18.pdf`
- `DC-A588工控主机规格说明书2023-1-2.pdf`
- `dtb-from-official-image/`
- `dtb-on-board/`
- `image-unpack/`
- `images-thirdparty/`

#### 已观察到的设备树文件

- `rk3588-evb7-lp4-v10.dtb`
- `rk3588-evb7-lp4-v10.dts`
- `extracted.dts`

#### 初步判断

- 该仓库适合作为后续：
  - 参考镜像来源
  - 对照 dts / dtb
  - 查看 Loader、分区、镜像结构
  - 追踪第三方适配尝试

---

### 2. Ubuntu Rockchip 支持请求

**Joshua-Riek/ubuntu-rockchip issue #1258**  
<https://github.com/Joshua-Riek/ubuntu-rockchip/issues/1258>

#### 标题

- `Feature Request: Support for 定昌 DC-A588 (RK3588-based) Board`

#### issue 主体中的关键信息

- 有人已经提交该板子的支持请求
- 提供了：
  - Archive-DC-A588 仓库链接
  - 从官方镜像提取的 dtb
  - 官方中文文档链接
- 反馈称：
  - Lemon1151 的 Ubuntu Rockchip 分支在 kernel 6.1.x 下运行较好
  - 显示效果优于官方 Ubuntu / Debian 镜像

#### 当时列出的已知问题

- EC20 4G 模块不识别
- TF 卡读卡器不识别
- 安装界面背景闪烁
- HDMI 无法达到原生超宽分辨率
- Chromium 启动异常

#### 对当前排查的意义

- 说明这块板子已经有人做过 Linux 适配尝试
- 证明“官方镜像 / 第三方镜像 / 设备树提取”这条路线是存在的

---

### 3. Armbian 社区适配讨论

**ophub/amlogic-s9xxx-armbian issue #2988**  
<https://github.com/ophub/amlogic-s9xxx-armbian/issues/2988>

#### 这个 issue 的价值

这基本是一份现成的板子适配/踩坑记录，信息量非常大。

#### 已知硬件识别信息（非常关键）

后续讨论中给出了更明确的硬件型号：

- 有线网卡 1（PCI）：Realtek `RTL8211F`
- 有线网卡 2（USB）：Realtek `RTL8153`
- 无线模块：SparkLAN `AP6275P`
- 无线芯片：Broadcom `BCM43752`

#### 这解释了什么

- 为什么双网口行为可能不同
- 为什么某些镜像一个网口可用、另一个异常
- 为什么无线支持高度依赖内核 / dts / 固件组合

#### 讨论中出现的关键结论

- 串口日志已确认板子模型：
  - `Model: DCZTL DC A588`
- 可通过 TTL 串口看到完整启动日志
- 某些 Armbian / Ubuntu 第三方镜像可以启动
- 某些版本下：
  - 有线网络正常
  - Wi-Fi 正常
  - 温度正常
- 但也存在版本差异导致的问题：
  - TF 卡不工作
  - EC20 不工作
  - OTG 不工作
  - U-Boot 阶段网卡未识别
  - 某些 HDMI 口输出异常或分辨率不正确

#### 讨论中的关键日志特征

- U-Boot 阶段可能出现：
  - `Net: No ethernet found.`
- Linux 内核阶段又可能看到：
  - `RTL8211F Gigabit Ethernet`
  - 说明 U-Boot 与 Linux 阶段的网卡初始化不一定一致

#### 讨论中对 OTG/TF 的重要提示

- 有人认为：
  - OTG 和 TF 问题与设备树不完整有关
- 后续有人反馈：
  - 上面的 OTG 口问题已被部分解决
  - TF 卡和 EC20 仍在继续研究

#### 对当前排查的意义

- 说明我们的观察并不是孤例
- 说明这块板子在 Linux/Armbian 适配上确实有现成经验可参考
- 也说明很多问题不是“硬件坏”，而是：
  - dts 不完整
  - u-boot 配置不匹配
  - vendor kernel / 社区 kernel 差异

---

## 二、当前能提炼出的综合判断

### 1. 板子命名

当前资料中常见命名：

- `DC-A588`
- `DC-A588D5-V01`

推测：

- `DC-A588`：系列 / 机型名
- `DC-A588D5-V01`：更具体的板卡版本 / 子型号

### 2. 社区支持状态

- 不是完全没人碰过
- 但资料分散在：
  - 厂商 PDF / wiki
  - GitHub 资料归档
  - Ubuntu Rockchip issue
  - Armbian issue
  - 第三方 patch / fork

### 3. 后续真正关键的不是“有没有资料”，而是“哪套组合最匹配”

核心组合包括：

- U-Boot
- DTS / DTB
- 内核
- Vendor 驱动 / 固件

很多外设问题（双网口、OTG、TF、EC20、Wi-Fi）都与这套组合强相关。

---

## 三、对当前项目的直接帮助

### 已被外部资料强化确认的事项

- DEBUG 串口路线正确
- BOOT / Loader / 刷机模式路线正确
- 双网口现象合理
- USB Host / OTG 的区分非常关键
- TF / EC20 / OTG 可能在不同镜像下表现不一致

### 后续使用这些资料时的建议

1. 把外部资料作为“参考基线”，不要盲信完全匹配
2. 对每一套镜像 / dtb / u-boot 组合单独记录行为
3. 特别关注：
   - 哪个版本 OTG 可用
   - 哪个版本双网口正常
   - 哪个版本 Wi-Fi 正常
   - 哪个版本 TF/EC20 仍异常

---

## 四、值得继续深挖的外部方向

### 1. Archive-DC-A588 仓库内部文件

建议后续进一步分析：

- `image-unpack/` 里的镜像结构
- `dtb-from-official-image/` 与 `dtb-on-board/` 差异
- `images-thirdparty/` 中有哪些可直接参考的镜像或命名

### 2. 厂商 wiki

Issue 中提到：

- `http://wikicn.gzdcsmt.com/wendang_id_8.html#son36`

后续可尝试通过浏览器手动查看，看是否存在：

- 更详细的刷机教程
- OTG 使用说明
- Android / Ubuntu / Debian 镜像下载页
- 更准确的板级资料

### 3. Lemon1151 相关适配

已有 issue 明确提到该方向对 DC-A588 有参考价值：

- U-Boot patch
- Linux DTS / DTSI
- Ubuntu Rockchip 适配分支

这部分后续值得单独整理。

---

## 五、当前最有价值链接清单

- Archive 仓库：
  - <https://github.com/AndroidOL/Archive-DC-A588>
- Ubuntu Rockchip 支持请求：
  - <https://github.com/Joshua-Riek/ubuntu-rockchip/issues/1258>
- Armbian 适配讨论：
  - <https://github.com/ophub/amlogic-s9xxx-armbian/issues/2988>

