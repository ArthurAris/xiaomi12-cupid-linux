# AGENTS.md — 小米12 主线 Linux 移植（终版规则）

> 项目规则与已确认决策。不记录进度；当前状态和限制见 `docs/known-issues.md`。

## 设备与目标

- 设备：小米12（2201123C），代号 **cupid**，SoC **SM8450**（骁龙 8 Gen 1，Adreno 730）。
- 面板 1080x2400 AMOLED（60/90/120Hz，本方案只暴露 120Hz）；A/B 双槽；
  `boot_a/b` 192 MiB；`recovery_a/b` 100 MiB（ramdisk-only TWRP）；`userdata` 222 GB。
- 目标：主线 Linux + Ubuntu/Phosh 日用；当前为**单系统**（`boot_a` 直接启动 Linux）。

## 仓库结构

```
docs/                  架构、复现手册、已知问题、硬件、面板资料
patches/kernel/        内核 patch + 新增文件 + config
tools/                 构建、打包、刷写、部署、串口、验证
```

## 设备安全规则（必须遵守）

1. 设备同一时刻只有一个操作者；每条设备命令带 serial。
2. 风险分级递进：只读 → 内存启动 → 写分区 → 改 bootloader 状态。
3. 写分区前必须备份，双端哈希一致；恢复镜像在手才做持久写入。
4. A/B 槽状态只用 bootloader 自己的命令改（`fastboot set_active`），不手写属性位。
5. 破坏性命令只对状态已核实的设备发出；长时间试验前读电量（≥40%）。
6. 序列号、MAC、蓝牙地址、校准数据、备份、密码只放 `private/`，不进任何仓库。
7. 推送、建仓、发版、改可见性、重写历史，每一项先问用户。

## 已确认的决策

1. **单系统**：Linux 直接写 `boot_a`；Android 已移除（userdata 已格式化），
   回退靠 `private/backup/boot_a-current-*.img`。
2. **分区**：根 = `userdata`（`/dev/sda38`，ext4 label `linux`）；旧 `cust` 保留作 init 回退。
3. **发行版/桌面**：Ubuntu 26.04 arm64 + Phosh，中文界面。
4. **调试通道**：开发期串口 root 无密码 + USB 复合网络；发布前关闭或设密码。
5. **远程桌面**：wayvnc 0.5 + noVNC，无密码，仅局域网。
6. **发布**：个人使用；仓库公开，但不含备份、固件二进制和本机唯一数据。

## 不能进公开仓

- 本机唯一数据（序列号、MAC、蓝牙地址、校准、传感器配置）。
- 原厂固件二进制、ROM 解包、分区备份、镜像。
- 含个人信息的原始日志、WiFi SSID、照片。
- 任何 token、密码、私钥。

## 修改纪律

- 一次只改一个变量；先记录基线，再上机。
- 挂死/黑屏类问题优先保留可回退镜像；不要在电量低时做持久写入。
- 新结论写入 `docs/`，不要只留在聊天里。
