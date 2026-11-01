# tools

终版工具集。所有脚本都按“先备份、再写入、后校验”的顺序设计。

| 路径 | 用途 |
|---|---|
| `build-final.sh` | 构建内核 Image + boot.img（WSL/Linux） |
| `initramfs/build-initramfs.sh` | 生成 `CONFIG_INITRAMFS_SOURCE` 目录（busybox + init v21） |
| `initramfs/init_static_v21.c` | PID1：挂载、找根、切换 root、串口 shell、看门狗 |
| `mkbootimg/` | boot.img 打包/解包（AOSP mkbootimg） |
| `rootfs-phosh/` | Ubuntu/Phosh 根文件系统构建、overlay、部署后配置 |
| `deploy-rootfs.ps1` | 把根文件系统镜像部署到 `userdata`（备份/哈希/电量门） |
| `linux-serial.ps1` | USB ACM 串口命令通道（身份校验 + 脚本上传） |
| `diagnostics/capture-desktop.sh` | 通过 Wayland 截取桌面 |
| `diagnostics/final-verify.sh` | 终版验证：内核、根、桌面、DSI、电池 |

## 典型流程

```bash
# 1. 内核 + boot.img
KERNEL_TREE=/root/linux ./tools/build-final.sh

# 2. initramfs 目录（如果还没建）
./tools/initramfs/build-initramfs.sh /root/initramfs-cupid-v26

# 3. 刷 boot_a（先备份！）
fastboot flash boot_a out/boot-final.img

# 4. 根文件系统
powershell -File tools/deploy-rootfs.ps1 -Image <rootfs.img>
```

串口调试：

```bash
powershell -File tools/linux-serial.ps1 -CommandFile <script.sh> -Label diag
```

## 注意

- `deploy-rootfs.ps1` 和 `linux-serial.ps1` 从 `private/device-target.json` 读取
  serial/adb 路径；该文件不进仓库。
- 所有脚本默认拒绝覆盖已有产物；需要时先手动清理。
- 不要用 Windows 侧分叉的内核树构建，实际验证都在 WSL `/root/linux`。
