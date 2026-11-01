# 小米12 (cupid) 主线 Linux 移植 — 终版

把主线 Linux + Ubuntu/Phosh 跑在 **Xiaomi 12（2201123C，代号 cupid，SM8450 / 骁龙 8 Gen 1）**
上的完整方案：文档、内核/U-Boot 补丁、构建与部署工具。

> 个人项目，**不提供担保**。刷写 bootloader 分区有变砖风险；操作前务必备份并能回退。

## 当前状态

- **单系统 Linux**：`boot_a` 直接启动内核 `7.2.8-cupid+`，开机进桌面，不需要电脑。
- **根文件系统**：原 222 GB `userdata`（`/dev/sda38`，ext4 label `linux`），可用约 213 GiB。
- **桌面**：Ubuntu 26.04 arm64 + Phosh，中文界面，902 个包。
- **已真机验证**：物理显示（120Hz、DSC max_qp=8）、触摸、GPU 合成、三簇调频、热保护、
  ADSP 电池、电源/音量键、USB 复合网络、WiFi 2.4G/5G 扫描、NTP、Chromium 156（软件渲染）、
  xfce4-terminal、wayvnc + noVNC 浏览器远程桌面。
- **未完成**：音频、蓝牙、GPS、自动旋转、系统挂起、60/90Hz 切换、5G 校准、
  GPU 动态内容横条噪声、面板完全自初始化。详见 [docs/known-issues.md](docs/known-issues.md)。

## 文档

| 文件 | 内容 |
|---|---|
| [docs/architecture.md](docs/architecture.md) | 启动链、显示、电源、网络、根文件系统的最终设计 |
| [docs/porting-playbook.md](docs/porting-playbook.md) | 从零复现：构建、打包、刷入、回退、踩坑清单 |
| [docs/known-issues.md](docs/known-issues.md) | 当前限制、规避方式、后续方向 |
| [docs/hardware-list.md](docs/hardware-list.md) | 硬件清单（面板/触控/音频/WiFi/充电/传感器） |
| [docs/panel-commands.md](docs/panel-commands.md) | 面板 DCS 命令表 |
| [docs/panel-timings.md](docs/panel-timings.md) | 面板时序 |
| [docs/reference/](docs/reference/) | 运行时 DT compatible 全表、Novatek DSC 取证 |

## 仓库结构

```
docs/                  终版文档（架构、复现手册、已知问题、硬件、面板）
patches/kernel/        内核改动：相对基线 commit 的 patch + 新增源文件 + v78 config
tools/                 构建、打包、刷写、部署、串口与验证脚本
  build-final.sh         内核 + boot.img 一键构建
  initramfs/             init v21 源码与 initramfs 构建脚本
  mkbootimg/             boot.img 打包工具
  rootfs-phosh/          Ubuntu/Phosh 根文件系统构建与 overlay
  deploy-rootfs.ps1      根文件系统部署（含备份/校验/电量门）
  linux-serial.ps1       串口调试通道
  diagnostics/           桌面截图与终版验证
AGENTS.md              项目规则与安全门
```

**没有上传完整内核树**：改动以 patch + 新文件提供，基线 commit 见
`patches/kernel/base.txt`。U-Boot/双系统路线已放弃，相关补丁未包含。
也没有上传备份、固件二进制、ROM 解包和本机唯一数据。

## 快速开始

```bash
# 1. 内核树切到基线（见 patches/kernel/base.txt），应用补丁
git apply patches/kernel/0001-cupid-kernel-changes.patch
cp -a patches/kernel/new-files/. .
cp patches/kernel/config-v78 .config

# 2. 构建（WSL/Linux）
KERNEL_TREE=/path/to/linux ./tools/build-final.sh

# 3. 备份 boot_a 后刷入
fastboot flash boot_a out/boot-final.img

# 4. 部署根文件系统（需先进入 TWRP 或 Linux）
powershell -File tools/deploy-rootfs.ps1 -Image <rootfs.img>
```

完整步骤、偏移量、回退方法见 [docs/porting-playbook.md](docs/porting-playbook.md)。

## 关键事实（避免重复踩坑）

- ABL 只认 v2 header；DTB 身份必须是 `msm-id <0x1c9 0x10000>` / `board-id <0 0>`；
  boot.img 的 dtb 字段放原厂多 DTB 容器当“诱饵”，内核用内建 DTB 覆盖。
- 内建 DTB 后 cmdline 只来自 DTB `/chosen/bootargs`，initramfs 必须编进内核。
- 面板 DSC 第 0 档 `max_qp = 8`；只暴露 120Hz。
- 电池属性 id：`BATT_CONSTANT_CURRENT` 在 12（温度在 13）。
- RTC 写被 SPMI 封锁，校时只能 NTP。
- Chromium 必须 `--disable-gpu`；wayvnc 用 0.5（wlr-screencopy）。

## 参考

- [xiaomi13-fuxi-linux-ubuntu26.10-kernel7.3](https://github.com/ilovecnxx/xiaomi13-fuxi-linux-ubuntu26.10-kernel7.3)
- [iLtanaka marble 集群](https://github.com/iLtanaka)（SM7475，postmarketOS + U-Boot chainload）
- `android-mainline-port` skill（流程与纪律）

## 许可

文档和脚本暂未指定许可证（作者保留权利）。`patches/` 中的代码派生自 Linux 内核
（GPL-2.0）与 U-Boot（GPL-2.0+），分发时遵循其原始许可证。
