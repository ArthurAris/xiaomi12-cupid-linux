# rootfs-phosh

在 WSL 里离线构建 Ubuntu 26.04 arm64 + Phosh 根文件系统。

## 前置

- 基础 rootfs（minbase）解包在 `/root/rootfs-arm64`；
- 目标目录 `/root/rootfs-phosh-v1`（脚本拒绝覆盖已存在目录）；
- 以 root 运行（WSL）。

## 流程

```bash
# 1. 复制基础 rootfs（保留权限），生成 base-protected.sha256
./prepare-rootfs.sh

# 2. 在隔离副本里装包（清华 ubuntu-ports 源）
./install-packages.sh

# 3. 应用 overlay/：locale、PAM、Phosh 服务、policy-rc.d、gschema
./configure-rootfs.sh

# 4. 打包成镜像（任选方式），部署到设备 userdata
#    见 tools/deploy-rootfs.ps1

# 5. 部署后应用 overlay-v7：网络/NTP/polkit/WiFi 省电
sudo ./apply-v7.sh /path/to/rootfs.img
```

## 目录

| 路径 | 内容 |
|---|---|
| `prepare-rootfs.sh` | 复制 base → rootfs-phosh-v1，安装 policy-rc.d，记录受保护文件哈希 |
| `install-packages.sh` | 按 `packages.list` 安装包，输出 `install-plan.txt` |
| `configure-rootfs.sh` | 应用 `overlay/`，校验 PAM/服务/schema |
| `overlay/` | 基础配置：rootfs 标记、locale、PAM、phoc.ini、SSH 限制、Phosh 单元、gschema |
| `overlay-v7/` | 部署后配置：USB 网络（无默认路由）、NTP 定时器、polkit、WiFi 省电 |
| `apply-v7.sh` | 把 overlay-v7 应用到镜像；禁用 RTC 写定时器 |
| `packages.list` | 基础包清单 |
| `base-protected.sha256` | 基础 rootfs 受保护文件的哈希 |
| `validate-schema.py` | gschema 校验 |
| `check-pam-permissions.py` | PAM/setuid 权限检查 |
| `in-rootfs.sh` | chroot 执行辅助 |

## 注意

- `overlay-v7` 的 `80-cupid-usb.network` **不能加 Gateway**，否则 USB 网络会抢占默认路由。
- `cupid-rtc-save.timer` 必须保持 disabled：本板 RTC 写被 SPMI arbiter 封锁。
- `apply-v7.sh` 只改镜像，不接触运行中的设备。
