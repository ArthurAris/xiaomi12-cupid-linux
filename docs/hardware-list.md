# 小米12 (cupid) 硬件清单 —— 设备树取证定稿

> 证据来源：真机运行时设备树（private/device-tree.tar 36.5MB 全节点 + fdt.dtb），2026-10-27 取自 Qn_miao 内核（vendor=官方 OS3.0.3.0.VLCCNXM）。
> 989 个 compatible 节点全表：`docs/reference/dt-compatibles.txt`。状态：全部 Observed。

## 平台

- 根 model：**"Cupid based on Qualcomm Technologies, Inc SM8450"**
- 串口调试口：`/soc/qcom,qup_uart@99c000`，115200n8（bootargs `console=ttyMSM0`）→ **早期日志可走 UART**（比 pstore 还早）
- bootargs 全文已存（private/dt/chosen/bootargs）：kpti=0 swiotlb=noforce pcie_ports=compat 等

## 显示（点亮主线的主战场）

| 件 | 设备树节点 | 说明 |
|---|---|---|
| 面板 | `soc/qcom,mdss_mdp@ae00000/qcom,mdss_dsi_l3_42_02_0a_dsc_cmd` | **l3_42_02_0a**，DSC 压缩、command 模式，1080x2400@60/90/120Hz（modes 已核） |
| 供电 | `dsi_panel_pwr_supply*` 一族（avdd/extvdd/L1/L2/L3/prim_L18/sec_L18/**seeya**/sim） | 多批次面板供电方案全在 DT；`seeya` 变体 → 面板供应商 **SeeYA**；L18=LABIBB 偏置 |
| 面板 GPIO | pinctrl `display_panel_avdd_default` / `display_panel_avee_default` | OLED 偏置电源使能脚 |
| 常供 | `soc/display_panel_vci`（regulator-fixed） | VCI 常开 |
| DSI | `mdss_dsi_ctrl0/1` (qcom,dsi-ctrl-hw-v2.6) + `dsi-phy-v4.3` ×2 | 双 DSI 控制器，用 0 |
| KMS | `qcom,mdss_mdp` = qcom,sde-kms | SDE/DPU |
| 背光 | panel0-backlight（drm sysfs 已见） | — |

## 触控

| 件 | 节点 | 说明 |
|---|---|---|
| 主触控 | `soc/spi@990000/fts@0` = **st,spi（ST FTS 系列，SPI 总线）** | 传输层走 SPI；固件下载序列待从厂商源码取证 |
| 小米触控特性 | `soc/xiaomi-touch` = xiaomi-touch | 采样率/游戏模式等私有接口 |
| （存疑） | `soc/ssusb@a600000/synaptics_tcm@20` | synaptics,tcm-i2c 挂在 USB 节点下，疑似残留/特殊形态，待核 |

## 音频（首次出声走保守增益）

| 件 | 节点 | 说明 |
|---|---|---|
| 功放 | `i2c@880000/cs35l41@40` + `cs35l41@42` = **Cirrus CS35L41 ×2**（I2C 地址 0x40/0x42） | 双扬声器，与 marble 的 AW88263S 不同，但上游 cirrus/cs35l41 驱动现成 |
| Codec | `wcd938x-codec`（WCD938x） | 上游 wcd938x 驱动 |
| 宏 | rx/tx/va/wsa/wsа2-macro（lpass-cdc） | 上游 sm8450 音频拓扑同款 |
| 扬声器（存疑） | `wsa883x` ×4（wsa/wsа2 各 2） | 可能是参考设计残留，以 CS35L41 为准，待核 |
| 麦克风 | swr-dmic ×4（va_swr_master） | 数字麦 |

## 振动

- `awinic,aw8697_haptic`（i2c@884000/aw8697_haptic@5A）= **AW8697 线性马达驱动 IC**
- `qcom,pm8350b-swr-haptics`（SoundWire haptics，另一路）—— 两路并存，以 AW8697 为准待核

## 无线

| 件 | 节点 | 说明 |
|---|---|---|
| Wi-Fi/BT | **QCA6490**：`soc/bt_qca6490`(qcom,qca6490) + `soc/qcom,cnss-qca6490@b0000000` | **与 iLtanaka marble 完全同款** → marble 的 ath11k 固件/board-file 经验整组复用 |
| PCIe | pcie@1c00000 / pcie@1c08000（qcom,pci-msm）+ pcie-ep | Wi-Fi 挂 PCIe |

## 电源/充电

| 件 | 节点 | 说明 |
|---|---|---|
| 充电 PMIC | **PM8350B**（spmi） | battery-charger 走 `qcom,pmic_glink` |
| 并充 | **SMB1394** ×2（spmi@1 debug 节点见到） | 快充（67W 私有协议仍在，首版只读） |
| 电量 | qcom,battery-charger（pmic_glink） | 只读先行 |
| GPIO 扩展 | pm8008i / pm8008j（i2c@994000） | 面板/外设使能脚多挂这 |

## 传感器 / 生物

- 传感器：**SSC/SLPI**（`remoteproc-adsp@03000000` waipio-adsp-pas + `remoteproc-slpi@02400000` waipio-slpi-pas + qcom,msm-ssc-sensors + qmi-ts-sensors）→ hexagonrpc+libssc 路线（marble 同款）
- 指纹：`goodix,fingerprint`（soc/fingerprint_goodix）—— 首版不支持
- 相机：cci0/cam-sensor0/1 —— 首版不支持

## 对第 3 步的意义

1. 最小板级 DTS 的每个值都有出处：面板节点全路径、供电方案、pinctrl、UART console、触控 SPI 总线、CS35L41 I2C 地址。
2. Wi-Fi/传感器栈 = marble 同款，直接拿 iLtanaka 的整组经验（固件 board.bin、hexagonrpcd、libssc）。
3. 早期日志三通道全通：**UART（ttyMSM0）** > pstore/ramoops（待查保留区）> netconsole。
