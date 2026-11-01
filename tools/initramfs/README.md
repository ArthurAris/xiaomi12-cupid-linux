# initramfs（PID1 = init_static_v21）

内核用 `CONFIG_INITRAMFS_SOURCE` 把该目录编进 Image；boot.img 不带 ramdisk。

## 根发现顺序

1. GPT `PARTNAME=userdata`，且 ext4 卷标为 `linux`；
2. 回退 GPT `PARTNAME=cust`，固定 4194304 扇区，且卷标为 `linux`。

两者都要求 ext4 超级块有效，并校验 `/etc/cupid-rootfs-release` 标记文件。
找到后：挂载 → `mount --move` → `switch_root` → systemd。

## 其它行为

- 挂载 `/proc`、`/sys`、`/dev`、`/dev/pts`、`/run`、`/tmp`。
- 注册 USB 复合 gadget：ACM 串口 + RNDIS（`1d6b:0104`）；
  `usb0` 固定 `192.168.137.2/24`、`172.16.42.2/24`，**不设默认路由**。
- 启动 240 秒看门狗；systemd 写 `/run/cupid-boot-ok` 后交接。
- 串口控制台：`root` 无密码（仅开发期）。

## 构建

```bash
./tools/initramfs/build-initramfs.sh /root/initramfs-cupid-v26
```

脚本会：
- 创建目录结构；
- 安装静态 aarch64 busybox（没有则下载）；
- 用 `aarch64-linux-gnu-gcc -static` 编译 `init_static_v21.c`。

然后把内核 `.config` 的 `CONFIG_INITRAMFS_SOURCE` 指向该目录并 `make olddefconfig`。

## 修改注意

- PID1 绝不能退出；所有错误路径都要 `reboot()` 或进入循环。
- 不要依赖 PATH；所有命令用绝对路径或直接 syscall。
- 改动根发现逻辑时，同时保留 `cust` 回退，避免新根损坏后无法启动。
