# 固件

这一版用 **ESP-IDF** 和 **FreeRTOS**，芯片是 ESP32-S3。第一次烧录默认不接雷达：程序自己造一个从面前走过的人，眼睛会跟着转，用来先看屏幕亮不亮。

真雷达、手腕晃动、颜色和左右偏移，都还没在这块板上跑过。下面「真机上还要看什么」列出了第一次上电要核对的日志。

## 准备

- 开发板：Waveshare **ESP32-S3-Touch-AMOLED-1.75**，标准版或带壳的 `-B`。`-G` 的 GPIO17 / GPIO18 可能已经接到板上的 GPS。
- 一根能传数据的 USB-C 线。这个口是芯片自带的 USB，日志也从这里出来。
- ESP-IDF **v5.5.5**。微雪的入门说明里写过 v5.5.5 和 v6.0.2。本仓库在 v5.5.5 上 `idf.py build` 通过。屏驱动组件 `espressif/esp_lcd_co5300` 2.2.0 要求 IDF 5.4 或更新。

## 安装 ESP-IDF

按你的系统打开乐鑫的中文安装页，版本选 v5.5.5，芯片选 ESP32-S3：

https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5.5/esp32s3/get-started/index.html

装的时候把 ESP32-S3 的工具链勾上。装完以后，每开一个新终端都要先进工具环境。

Linux / macOS，在你解压 ESP-IDF 的那个目录里：

```bash
. $HOME/esp/esp-idf/export.sh
```

Windows 用「ESP-IDF 5.5 PowerShell」快捷方式，或者在 PowerShell 里：

```powershell
%userprofile%\esp\esp-idf\export.ps1
```

提示里出现工具链路径就可以了。乐鑫的 VS Code 插件也行，扩展里把 ESP-IDF 指到 v5.5.5。

## 编译和烧录

```bash
cd firmware
idf.py set-target esp32s3
idf.py build
idf.py -p PORT flash monitor
```

`PORT` 换成电脑认到的口。Linux 常见 `/dev/ttyACM0`，macOS 常见 `/dev/cu.usbmodem...`，Windows 是 `COMx`。

`sdkconfig.defaults` 已经写好，一般不用再进 `menuconfig`：Flash 16 MB、DIO、80 MHz（和微雪出厂烧录参数一致），CPU 240 MHz，八线 PSRAM，日志走 USB，应用程序分区 3 MB。

电脑看不到端口时：按住板子上的 **BOOT**，点一下 **RESET**，松开 BOOT，再执行上面的 `flash`。

退出监视器按 `Ctrl+]`。

## 第一次烧进去会看到什么

`firmware/main/config.h` 里 `SIMULATE_RADAR` 默认是 `1`。UART1 先不打开。每 100 ms 造一帧，帧格式和真雷达一样，再交给同一个解析函数。

- 槽 0：正前方 300 mm。距离小于 0.5 m，当成戴着手表的人，丢掉。
- 槽 1：前方大约 1.8 m，左右来回走。眼睛跟这一路。
- 槽 2：空的。

监视器大约每 200 ms 一行。`track` 是正在跟人，`slot=1` 是选中的槽。后面还有每个目标的 x、y 和速度，单位是毫米和厘米每秒。开机还会打 `LD2450 self-test passed`：用协议手册里的那帧例程对过解码（x = -782 mm，y = 1713 mm，速度 = -16 cm/s）。

屏幕上是黑底的一张简单脸：两个白眼、虹膜、瞳孔、高光，隔几秒眨一下。人走丢之后会停大约 1.5 秒，再慢慢看回中间，偶尔瞥一眼。颜色和位置在 `firmware/main/face.h` 开头的 `k_face`。换一张脸就改这个结构和 `face.c` 里的 `face_draw()`。

## 接上真雷达以后

雷达按仓库根目录的接线图焊好，再把 `config.h` 里这一行改成 0，重新 `idf.py build`，再烧录：

```c
#define SIMULATE_RADAR 0
```

这一行包在 `#ifndef SIMULATE_RADAR` 里。若要在不改数字的情况下关掉仿真，可以在 `firmware/main/CMakeLists.txt` 的 `idf_component_register` 后面加：

```cmake
target_compile_definitions(${COMPONENT_LIB} PRIVATE SIMULATE_RADAR=0)
```

雷达串口 256000，8 数据位，无校验，1 停止位。板子用 UART1：

| 信号 | GPIO | 接到 |
| --- | ---: | --- |
| 板子发送 TX | 17 | 雷达 RX |
| 板子接收 RX | 18 | 雷达 TX |

GPIO 编号以这两根线为准。排针第几脚写在 [hardware/README.md](../hardware/README.md)：本仓库原理图按官方 PDF 把 GPIO17 放在 H2 脚 7、GPIO18 放在脚 8。GitHub 上的 `HARDWARE_REFERENCE.md` 把脚 4 到脚 8 排成另一套。焊之前用万用表从芯片量到排针。

## 四个任务

都在 `app_main` 里创建。

| 任务 | 核 | 做什么 |
| --- | ---: | --- |
| `radar` | 0 | 仿真造帧，或者从 UART1 收字节。拼出一帧就放进队列 |
| `imu` | 0 | 每 20 ms 读 QMI8658。手腕动不动，用互斥锁交给选目标的任务 |
| `eye` | 0 | 每 33 ms 选一个人，平滑瞳孔位置，再交给显示 |
| `disp` | 1 | 在 PSRAM 里画两张图，轮流把眼睛那一条刷到屏上 |

雷达队列和眼睛队列深度都是 1。新的一帧会盖掉还没被读走的旧帧。IMU 读失败时当作手腕静止，眼睛继续动，监视器里会有一行警告。

手腕在动的时候不追人，眼睛看中间，偶尔眨、偶尔瞥。人丢掉之后先停约 1.5 秒，再漂回中间。距离、角度和平滑时间都在 `config.h`。

## 引脚依据

显示和 I2C 这三处一致，没有另猜：

- 微雪 Arduino `examples/arduino/libraries/Mylibrary/pin_config.h`
- 同一仓库的 ESP-IDF BSP 头文件 `bsp/esp32_s3_touch_amoled_1_75.h`
- `HARDWARE_REFERENCE.md`

仓库：https://github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75

| 功能 | GPIO |
| --- | --- |
| LCD D0–D3 | 4、5、6、7 |
| LCD SCLK | 38 |
| LCD CS | 12 |
| LCD RST | 39 |
| I2C SDA / SCL | 15 / 14，400 kHz |

屏是 CO5300，QSPI，466×466。列起始偏移 6，和官方例程 `01_HelloWorld` 里 `Arduino_CO5300(..., 6, 0, 0, 0)` 一样。驱动每次刷图都会重写列窗口，所以固件调用 `esp_lcd_panel_set_gap(panel, 6, 0)`，坐标本身不再加 6。亮度是命令 `0x51`，`config.h` 里 `LCD_BRIGHTNESS_PCT` 为 63。这块屏没有背光脚。

I2C 地址：QMI8658 是 `0x6B`，AXP2101 是 `0x34`。IMU 的复位和量程按 SensorLib 来（微雪的 QMI8658 例程用的就是这个库）：加速度 ±4 g，陀螺 ±512 dps。AXP2101 只读芯片号寄存器 `0x03`，期望 `0x4A`，不改充电电流。

雷达帧格式来自海凌科《LD2450 串口通信协议 V1.03》第 2.3 节：帧头 `AA FF 03 00`，三个目标各 8 字节，帧尾 `55 CC`。符号位的规定和普通补码不一样，解码写在 `ld2450.c`。

电脑上可以先跑解析和选目标的自测（不需要板子）：

```bash
gcc -std=c11 -Wall -Wextra -I firmware/main -o /tmp/test_logic \
  firmware/test/test_logic.c firmware/main/ld2450.c firmware/main/gaze.c -lm
/tmp/test_logic
```

打印 `logic ok` 就表示手册例程、仿真帧和「丢掉近处的人」这几条通过了。

## 这一版编译出来有多大

ESP-IDF v5.5.5，`idf.py size`（2026-10-21 这次编译）：

| 项目 | 大小 |
| --- | --- |
| 应用程序镜像 | 274077 字节（烧录文件 `eyeballer-seeker.bin` 为 0x42F10，约 268 KB） |
| 3 MB 应用程序分区还剩 | 约 91%（0x2BD0F0 字节） |
| 片内 RAM（DIRAM） | 用了 76607 字节，还剩 265153 字节（总共 341760） |
| 指令 RAM（IRAM） | 16384 / 16384，乐鑫启动代码会把它占满 |
| Flash 代码 + 只读数据 | 124226 + 59356 字节 |

两张全屏缓冲不在上面的静态表里。运行时向八线 PSRAM 各要 466×466×2 = 434312 字节，一共约 848 KB。显示任务只反复刷新眼睛那一条（大约第 90 行到第 290 行），目标是 30 帧每秒。监视器里 `display fps=` 是板上实测，这台编译机没有屏幕，没有测到这个数。

## 真机上还要看什么

1. 监视器里有 `LD2450 self-test passed`、`CO5300 466x466`。I2C 正常时还有 `QMI8658 online` 和 `AXP2101 online, chip id 0x4A`。缺 IMU 或电源芯片时程序继续跑，眼睛仍会动。
2. 脸在圆屏正中，左右没有被切掉。若整张脸偏一列，先看列偏移是不是 6。颜色反了再看 RGB565 的字节顺序。
3. 仿真模式下眼睛左右扫。改成 `SIMULATE_RADAR 0` 后，0.5 m 以内的人不被选中，更远的走动目标被选中；人离开后大约 1.5 秒才回到中间。
4. 拿着板子晃，眼睛回到中间；放稳以后继续跟人。阈值在 `config.h` 的 `ACCEL_MOVE_G` 和 `GYRO_MOVE_DPS`，这台机器上没标定过。
5. `display fps=` 是否到 30 附近。
6. H2 脚序用万用表核对，再焊雷达。串口线上的 220Ω 是保护电阻。
