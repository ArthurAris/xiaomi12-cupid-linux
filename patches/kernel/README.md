# 内核改动（Linux）

本目录只放**改动**，不放完整内核树。

- `base.txt`：基线 commit 和当前 HEAD（patch 相对基线生成）
- `0001-cupid-kernel-changes.patch`：从基线到当前工作树的完整 diff（含已提交的板级支持）
- `new-files/`：diff 无法表达的新增文件，按内核树原路径存放
- `config-v78`：真机验证过的 v78 `.config`

## 基线

```
base_commit=ed120558cd2c5f970edc44e1273a8d7b6caed825
base_subject=ASoC: qcom: q6apm-voice: drop and free PCM on backend close
head_commit=5a1ff1c2d6f69dd7c92497f1655645dd383657d3
head_subject=arm64: dts: qcom: add initial Xiaomi 12 (cupid) board support
```

`5a1ff1c2d` 是本地提交（初版板级支持），不在上游；patch 以 `ed120558c` 为基准，
包含该提交和其后的所有工作树改动。

## 应用

```bash
cd /path/to/linux
git checkout ed120558c
git apply /path/to/patches/kernel/0001-cupid-kernel-changes.patch
cp -a /path/to/patches/kernel/new-files/. .
cp /path/to/patches/kernel/config-v78 .config
```

然后按 `docs/porting-playbook.md` 第 2–4 节构建。注意：

- `config-v78` 里 `CONFIG_INITRAMFS_SOURCE` 指向 `/root/initramfs-cupid-v26`，
  需要先按 playbook 生成该目录（init 源码在 `tools/initramfs/init_static_v21.c`）。
- 不要用 Windows 侧分叉的 `kernel/` 树构建；实际验证都在 WSL `/root/linux`。
- 板级 DTS 使用 `sm8450-xiaomi-cupid-v-touch.dts`，`CUPID_DTB` 必须与之一致。

## 主要改动

| 文件 | 内容 |
|---|---|
| `arch/arm64/kernel/cupid_early.c` / `cupid_dtb.S` / `cupid_font.h` | 早期日志通道、内建 DTB 覆盖、屏显字体 |
| `arch/arm64/kernel/setup.c` / `Makefile` / `Kconfig` | `CONFIG_CUPID_FORCE_BUILTIN_DTB` |
| `arch/arm64/boot/dts/qcom/sm8450-xiaomi-cupid.dts` | 板级 DTS（内存、UFS、PMIC、显示、触摸、WiFi、按键等） |
| `arch/arm64/boot/dts/qcom/sm8450-xiaomi-cupid-v-touch.dts` | v78 使用的最终板级 DTS |
| `arch/arm64/boot/dts/qcom/cupid-*.dtsi` | CPU OPP、WiFi、符号并集等 |
| `drivers/gpu/drm/panel/panel-xiaomi-cupid.c` / `-blobs.h` | 面板驱动（DSC max_qp=8、120Hz、息屏/亮屏、full_init） |
| `drivers/input/touchscreen/st-ftm5.c` | ST FTM5 触摸 |
| `drivers/power/supply/qcom_battmgr.c` | SM8450 属性 id 偏移、电流符号 |
| `arch/arm64/boot/dts/qcom/sm8450.dtsi` | 少量上游修正 |
