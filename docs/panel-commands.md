# 面板 MIPI 命令解码（l3_42_02_0a）

> 来源：运行时设备树 qcom,mdss-dsi-*-command 属性；格式=高通 dsi_ctrl_hdr(8B)+载荷+4B 对齐。
> 用途：面板驱动 on/off/timing-switch 序列直接对照。

## C:\Users\Administrator\Downloads\xiaomi12\out\dsi-blobs\t0_on-command.bin  (859B)
  0 DCS_SHORT_WRITE_2    vc=0 wait=0ms len=  2  90 01
  1 DCS_LONG_WRITE       vc=0 wait=0ms len= 19  91 AB 28 00 0C C2 00 03 1C 01 7E 00 0F 08 BB 04 3D 10 F0
  2 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  03 01
  3 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  2C 00
  4 DCS_SHORT_WRITE_2    vc=0 wait=0ms len=  2  35 00
  5 DCS_SHORT_WRITE_2    vc=0 wait=0ms len=  2  53 20
  6 DCS_LONG_WRITE       vc=0 wait=0ms len=  5  51 00 00 00 00
  7 DCS_LONG_WRITE       vc=0 wait=0ms len=  5  2A 00 00 04 37
  8 DCS_LONG_WRITE       vc=0 wait=0ms len=  5  2B 00 00 09 5F
  9 DCS_SHORT_WRITE_2    vc=0 wait=0ms len=  2  2F 01
 10 DCS_SHORT_WRITE_2    vc=0 wait=0ms len=  2  26 01
 11 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 01
 12 DCS_LONG_WRITE       vc=0 wait=0ms len=  8  C3 94 01 8C D0 22 02 00
 13 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 01
 14 DCS_LONG_WRITE       vc=0 wait=0ms len=  4  D2 00 00 11
 15 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 06
 16 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  D2 05
 17 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 0F
 18 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  D2 00
 19 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 09
 20 DCS_LONG_WRITE       vc=0 wait=0ms len=  3  D2 00 00
 21 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  CE 00
 22 DCS_LONG_WRITE       vc=0 wait=0ms len=  5  FF AA 55 A5 80
 23 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 61
 24 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  F3 80
 25 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 00
 26 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C0 46
 27 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 0B
 28 DCS_LONG_WRITE       vc=0 wait=0ms len=  3  B5 23 2B
 29 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 00
 30 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 29
 31 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  D9 00
 32 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 01
 33 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  D9 80
 34 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 07
 35 DCS_LONG_WRITE       vc=0 wait=0ms len=  3  B2 07 FF
 36 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 17
 37 DCS_LONG_WRITE       vc=0 wait=0ms len=  3  B2 07 FF
 38 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 1F
 39 DCS_LONG_WRITE       vc=0 wait=0ms len=  3  B2 00 50
 40 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 07
 41 DCS_LONG_WRITE       vc=0 wait=0ms len= 44  C9 21 00 27 D9 27 D9 00 00 3F E0 8E C6 3F E0 8E C6 80 06 33 D5 F1 00 16 13 00 7B 78 33 D0  …(+14B)
 42 DCS_LONG_WRITE       vc=0 wait=0ms len= 44  CA 27 00 27 D9 27 D9 00 00 00 00 00 00 3F E0 8E C6 80 00 00 00 00 00 7C 13 00 E1 78 33 D0  …(+14B)
 43 DCS_LONG_WRITE       vc=0 wait=0ms len= 44  CB 2D 00 27 D9 27 D9 00 00 3F E0 8E C6 80 00 00 00 00 00 00 00 00 00 16 79 00 7B DE 33 70  …(+14B)
 44 DCS_LONG_WRITE       vc=0 wait=0ms len= 44  CC 2B 00 27 D9 27 D9 00 00 00 00 00 00 80 00 00 00 7F F9 CC 2A 0F 00 7C 79 00 E1 DE 33 7F  …(+14B)
 45 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C1 00
 46 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C2 00
 47 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C3 00
 48 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C4 00
 49 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C5 00
 50 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C6 00
 51 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C7 00
 52 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C8 00
 53 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  CD 00
 54 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  CE 00
 55 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  CF 00
 56 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  D0 00
 57 DCS_LONG_WRITE       vc=0 wait=0ms len=  3  C0 05 02
 58 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 00
 59 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 05
 60 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  BE 0A
 61 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 00
 62 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 01
 63 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C0 B3
 64 DCS_LONG_WRITE       vc=0 wait=0ms len=  5  FF AA 55 A5 80
 65 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 2E
 66 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  FB D1
 67 DCS_SHORT_WRITE_1    vc=0 wait=80ms len=  2  11 00
 68 DCS_SHORT_WRITE_1    vc=0 wait=0ms len=  2  29 00
parse=OK 全部解析 共 69 条

## C:\Users\Administrator\Downloads\xiaomi12\out\dsi-blobs\t0_off-command.bin  (18B)
  0 DCS_SHORT_WRITE_1    vc=0 wait=20ms len=  2  28 00
  1 DCS_SHORT_WRITE_1    vc=0 wait=100ms len=  2  10 00
parse=OK 全部解析 共 2 条

## C:\Users\Administrator\Downloads\xiaomi12\out\dsi-blobs\t0_timing-switch-command.bin  (40B)
  0 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 02
  1 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  CC 00
  2 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  2F 01
  3 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  26 01
parse=OK 全部解析 共 4 条

## C:\Users\Administrator\Downloads\xiaomi12\out\dsi-blobs\t1_on-command.bin  (881B)
  0 DCS_SHORT_WRITE_2    vc=0 wait=0ms len=  2  90 01
  1 DCS_LONG_WRITE       vc=0 wait=0ms len= 19  91 AB 28 00 0C C2 00 03 1C 01 7E 00 0F 08 BB 04 3D 10 F0
  2 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  03 01
  3 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  2C 00
  4 DCS_SHORT_WRITE_2    vc=0 wait=0ms len=  2  35 00
  5 DCS_SHORT_WRITE_2    vc=0 wait=0ms len=  2  53 20
  6 DCS_LONG_WRITE       vc=0 wait=0ms len=  5  51 00 00 00 00
  7 DCS_LONG_WRITE       vc=0 wait=0ms len=  5  2A 00 00 04 37
  8 DCS_LONG_WRITE       vc=0 wait=0ms len=  5  2B 00 00 09 5F
  9 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 00
 10 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 09
 11 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C0 20
 12 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  2F 02
 13 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 01
 14 DCS_LONG_WRITE       vc=0 wait=0ms len=  8  C3 94 01 8C D0 22 02 00
 15 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 01
 16 DCS_LONG_WRITE       vc=0 wait=0ms len=  4  D2 00 00 11
 17 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 06
 18 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  D2 05
 19 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 0F
 20 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  D2 00
 21 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 09
 22 DCS_LONG_WRITE       vc=0 wait=0ms len=  3  D2 00 00
 23 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  CE 00
 24 DCS_LONG_WRITE       vc=0 wait=0ms len=  5  FF AA 55 A5 80
 25 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 61
 26 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  F3 80
 27 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 00
 28 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C0 46
 29 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 0B
 30 DCS_LONG_WRITE       vc=0 wait=0ms len=  3  B5 23 2B
 31 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 00
 32 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 29
 33 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  D9 00
 34 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 01
 35 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  D9 80
 36 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 07
 37 DCS_LONG_WRITE       vc=0 wait=0ms len=  3  B2 07 FF
 38 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 17
 39 DCS_LONG_WRITE       vc=0 wait=0ms len=  3  B2 07 FF
 40 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 1F
 41 DCS_LONG_WRITE       vc=0 wait=0ms len=  3  B2 00 50
 42 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 07
 43 DCS_LONG_WRITE       vc=0 wait=0ms len= 44  C9 21 00 27 D9 27 D9 00 00 3F E0 8E C6 3F E0 8E C6 80 06 33 D5 F1 00 16 13 00 7B 78 33 D0  …(+14B)
 44 DCS_LONG_WRITE       vc=0 wait=0ms len= 44  CA 27 00 27 D9 27 D9 00 00 00 00 00 00 3F E0 8E C6 80 00 00 00 00 00 7C 13 00 E1 78 33 D0  …(+14B)
 45 DCS_LONG_WRITE       vc=0 wait=0ms len= 44  CB 2D 00 27 D9 27 D9 00 00 3F E0 8E C6 80 00 00 00 00 00 00 00 00 00 16 79 00 7B DE 33 70  …(+14B)
 46 DCS_LONG_WRITE       vc=0 wait=0ms len= 44  CC 2B 00 27 D9 27 D9 00 00 00 00 00 00 80 00 00 00 7F F9 CC 2A 0F 00 7C 79 00 E1 DE 33 7F  …(+14B)
 47 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C1 00
 48 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C2 00
 49 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C3 00
 50 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C4 00
 51 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C5 00
 52 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C6 00
 53 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C7 00
 54 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C8 00
 55 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  CD 00
 56 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  CE 00
 57 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  CF 00
 58 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  D0 00
 59 DCS_LONG_WRITE       vc=0 wait=0ms len=  3  C0 05 02
 60 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 00
 61 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 05
 62 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  BE 0A
 63 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 00
 64 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 01
 65 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C0 B3
 66 DCS_LONG_WRITE       vc=0 wait=0ms len=  5  FF AA 55 A5 80
 67 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 2E
 68 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  FB D1
 69 DCS_SHORT_WRITE_1    vc=0 wait=80ms len=  2  11 00
 70 DCS_SHORT_WRITE_1    vc=0 wait=0ms len=  2  29 00
parse=OK 全部解析 共 71 条

## C:\Users\Administrator\Downloads\xiaomi12\out\dsi-blobs\t1_timing-switch-command.bin  (93B)
  0 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  2F 02
  1 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 00
  2 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 09
  3 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C0 20
  4 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 02
  5 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  CC 30
  6 DCS_LONG_WRITE       vc=0 wait=10ms len=  2  CE 01
  7 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 02
  8 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  CC 00
parse=OK 全部解析 共 9 条

## C:\Users\Administrator\Downloads\xiaomi12\out\dsi-blobs\t2_on-command.bin  (934B)
  0 DCS_SHORT_WRITE_2    vc=0 wait=0ms len=  2  90 01
  1 DCS_LONG_WRITE       vc=0 wait=0ms len= 19  91 AB 28 00 0C C2 00 03 1C 01 7E 00 0F 08 BB 04 3D 10 F0
  2 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  03 01
  3 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  2C 00
  4 DCS_SHORT_WRITE_2    vc=0 wait=0ms len=  2  35 00
  5 DCS_SHORT_WRITE_2    vc=0 wait=0ms len=  2  53 20
  6 DCS_LONG_WRITE       vc=0 wait=0ms len=  5  51 00 00 00 00
  7 DCS_LONG_WRITE       vc=0 wait=0ms len=  5  2A 00 00 04 37
  8 DCS_LONG_WRITE       vc=0 wait=0ms len=  5  2B 00 00 09 5F
  9 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  2F 03
 10 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 00
 11 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 0A
 12 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C0 01
 13 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 02
 14 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  CC 30
 15 DCS_LONG_WRITE       vc=0 wait=20ms len=  2  CE 01
 16 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 02
 17 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  CC 00
 18 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 01
 19 DCS_LONG_WRITE       vc=0 wait=0ms len=  8  C3 94 01 8C D0 22 02 00
 20 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 01
 21 DCS_LONG_WRITE       vc=0 wait=0ms len=  4  D2 00 00 11
 22 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 06
 23 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  D2 05
 24 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 0F
 25 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  D2 00
 26 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 09
 27 DCS_LONG_WRITE       vc=0 wait=0ms len=  3  D2 00 00
 28 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  CE 00
 29 DCS_LONG_WRITE       vc=0 wait=0ms len=  5  FF AA 55 A5 80
 30 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 61
 31 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  F3 80
 32 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 00
 33 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C0 46
 34 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 0B
 35 DCS_LONG_WRITE       vc=0 wait=0ms len=  3  B5 23 2B
 36 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 00
 37 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 29
 38 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  D9 00
 39 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 01
 40 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  D9 80
 41 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 07
 42 DCS_LONG_WRITE       vc=0 wait=0ms len=  3  B2 07 FF
 43 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 17
 44 DCS_LONG_WRITE       vc=0 wait=0ms len=  3  B2 07 FF
 45 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 1F
 46 DCS_LONG_WRITE       vc=0 wait=0ms len=  3  B2 00 50
 47 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 07
 48 DCS_LONG_WRITE       vc=0 wait=0ms len= 44  C9 21 00 27 D9 27 D9 00 00 3F E0 8E C6 3F E0 8E C6 80 06 33 D5 F1 00 16 13 00 7B 78 33 D0  …(+14B)
 49 DCS_LONG_WRITE       vc=0 wait=0ms len= 44  CA 27 00 27 D9 27 D9 00 00 00 00 00 00 3F E0 8E C6 80 00 00 00 00 00 7C 13 00 E1 78 33 D0  …(+14B)
 50 DCS_LONG_WRITE       vc=0 wait=0ms len= 44  CB 2D 00 27 D9 27 D9 00 00 3F E0 8E C6 80 00 00 00 00 00 00 00 00 00 16 79 00 7B DE 33 70  …(+14B)
 51 DCS_LONG_WRITE       vc=0 wait=0ms len= 44  CC 2B 00 27 D9 27 D9 00 00 00 00 00 00 80 00 00 00 7F F9 CC 2A 0F 00 7C 79 00 E1 DE 33 7F  …(+14B)
 52 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C1 00
 53 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C2 00
 54 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C3 00
 55 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C4 00
 56 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C5 00
 57 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C6 00
 58 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C7 00
 59 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C8 00
 60 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  CD 00
 61 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  CE 00
 62 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  CF 00
 63 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  D0 00
 64 DCS_LONG_WRITE       vc=0 wait=0ms len=  3  C0 05 02
 65 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 00
 66 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 05
 67 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  BE 0A
 68 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 00
 69 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 01
 70 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C0 B3
 71 DCS_LONG_WRITE       vc=0 wait=0ms len=  5  FF AA 55 A5 80
 72 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 2E
 73 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  FB D1
 74 DCS_SHORT_WRITE_1    vc=0 wait=80ms len=  2  11 00
 75 DCS_SHORT_WRITE_1    vc=0 wait=0ms len=  2  29 00
parse=OK 全部解析 共 76 条

## C:\Users\Administrator\Downloads\xiaomi12\out\dsi-blobs\t2_timing-switch-command.bin  (93B)
  0 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  2F 03
  1 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 00
  2 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  6F 0A
  3 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  C0 01
  4 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 02
  5 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  CC 30
  6 DCS_LONG_WRITE       vc=0 wait=13ms len=  2  CE 01
  7 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 02
  8 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  CC 00
parse=OK 全部解析 共 9 条

## C:\Users\Administrator\Downloads\xiaomi12\out\dsi-blobs\t0_nolp-command.bin  (49B)
  0 DCS_LONG_WRITE       vc=0 wait=0ms len=  6  F0 55 AA 52 08 00
  1 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  B2 18
  2 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  38 00
  3 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  2F 01
  4 DCS_LONG_WRITE       vc=0 wait=0ms len=  2  26 01
parse=OK 全部解析 共 5 条
