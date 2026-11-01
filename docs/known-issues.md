# 已知问题与限制

> 只列**当前终版**仍然存在的限制。每项给出影响、当前规避方式和后续方向。

## 会挂死设备（高风险）

| # | 问题 | 影响 | 规避 | 后续方向 |
|---|---|---|---|---|
| 1 | 切 60/90Hz 会整机挂死 | 需长按电源 10–15 秒 | 只暴露 120Hz（`panel-xiaomi-cupid.c`） | 查 DSI/DSC 模式切换与面板时序；恒定时钟 + 改 vtotal 已试过仍挂 |
| 2 | 系统挂起 `echo freeze` 后 USB 全无、不自恢复 | 需长按电源 | 不要用系统挂起；只用息屏（面板 0x28+0x10） | 恢复 `CONFIG_CPU_IDLE`、补回 `apps_rsc` power-domains、把 USB 设为 wakeup source |
| 3 | Chromium 带 GPU 加速启动会挂死 | 需长按电源 | 启动器固定 `--disable-gpu` | 定位 freedreno/msm 在 Chromium 合成负载下的挂死点（可能与 #1 同源） |

## 功能缺失

| # | 问题 | 状态 | 说明 |
|---|---|---|---|
| 4 | 音频（扬声器/麦克风） | 未实现 | 硬件为 CS35L41×2 + WCD938x，上游驱动存在；需 DTS 与固件 |
| 5 | 蓝牙 | 未实现 | WCN6855，走 UART；ath11k 只覆盖 WiFi |
| 6 | GPS | 未实现 | 需 GNSS/ADSP 链路 |
| 7 | 自动旋转 | 未实现 | 需 SSC/SLPI 传感器栈（hexagonrpc + libssc） |
| 8 | 指纹/相机 | 不支持 | 首版范围外 |
| 9 | 5G 信号弱 | 部分可用 | 通用 `board-2.bin` 缺本机 5G 校准；2.4G 正常。用本机 `bdwlan.elf` 直接生成 board 文件的方案曾导致 USB 全无，风险高 |

## 体验与稳定性

| # | 问题 | 影响 | 规避 |
|---|---|---|---|
| 10 | 远程桌面实际帧率约 15–30fps | 操作有延迟 | `-f 120` 只是上限；可降到 20–30 换更稳；有线/5G WiFi 更好 |
| 11 | VNC 无密码 | 局域网内任何设备可控制桌面 | 仅可信网络使用；要认证需换 wayvnc 0.9 + RealVNC Viewer（RA2）或 TLS 证书 |
| 12 | GPU 动态内容横条噪声 | 滑动状态栏/开关面板时出现 | 暂用静态场景；或改 pixman 软件渲染（CPU 占用高） |
| 13 | 面板依赖 bootloader 首次初始化 | 息屏期间必须保持面板供电，待机功耗偏高 | 不要断 vddio/vddd/vci；后续实现完整 prepare 自初始化 |
| 14 | PMIC RTC 写被 SPMI arbiter 封锁 | 重启后到 NTP 同步前时间可能回退 | `cupid-timesync.timer` 走 NTP；`cupid-rtc-save.timer` 保持 disabled |
| 15 | Mobile Settings 无中文 | 应用界面英文 | 上游 0.53 未带 zh_CN；main 分支也只有 57/329 条翻译 |
| 16 | snapd 不可用 | 无法安装 snap 应用 | 内核无 squashfs、无 AppArmor LSM；用 deb 包 |
| 17 | USB gadget 偶尔整体消失 | 串口和 RNDIS 都没了 | 长按电源重启；不要反复刷写 |
| 18 | 串口 root 无密码 | 物理接触即可登录 | 仅开发期；发布前关闭或设密码 |
| 19 | Android 已移除 | 无法直接回 Android | 用备份 `boot_a-current-*.img` 刷回；userdata 数据不可恢复 |

## 已解决（供参考，不需要再查）

- 物理花屏：Novatek DSC 第 0 档 `max_qp = 8`。
- 120Hz 闪烁：精确时钟 349670 kHz。
- 电池温度 200000 / 充电电流符号：SM8450 属性 id 偏移 + 符号修正。
- 息屏/亮屏：0x28+0x10 / 0x11+PPS+0x29，rails 保持。
- 触摸、电源键、音量键、WiFi 2.4G/5G 扫描、USB 网络、NTP、中文界面、Chromium（软件渲染）、
  xfce4-terminal、wayvnc+noVNC。
