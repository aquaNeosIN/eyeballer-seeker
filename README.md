# eyeballer-seeker

A wrist-worn screen with anime eyes that follow people walking by.

Sit with the face turned outward. Someone crosses in front of the camera and the eyes lock on; they leave, and the eyes drift back to the middle. From the side it still looks like the face is looking at you. That is the Mona Lisa effect. The person wearing it is behind the camera. A shape that fills the frame is ignored.

This tree is the **v2** prototype: an Alientek DNESP32S3 board, a kit OV5640 camera, and a Waveshare 1.69″ touch LCD. The eyes follow motion. They do not yet recognize a face.

## Versions

v1 and v2 are different machines. They do not share a display bus or a sensor.

| | v1 | v2 (this tree) |
| --- | --- | --- |
| Board | Waveshare ESP32-S3 Touch AMOLED 1.75 | 正点原子 DNESP32S3 (ESP32-S3) |
| Display | 1.75″ round, QSPI, CO5300, about 466×466 | Waveshare 1.69″, 240×280, ST7789V2, 4-wire SPI |
| Sensing | HLK-LD2450 24 GHz radar, up to 3 targets | Kit OV5640 on the board DVP connector. Motion, then the center of the strongest cluster |
| Other parts | QMI8658 IMU, 5V boost for the radar, a small carrier PCB | None of those. Breadboard wires are enough |
| Firmware | `firmware/` on the v1 branch | `firmware/` in this tree |
| Where to read it | Draft pull request [#1](https://github.com/aquaNeosIN/eyeballer-seeker/pull/1), branch `cursor/hardware-schematic-2838` | This branch |

v1 is still the radar-and-round-screen design. Opening or merging this v2 tree does not change that pull request. A longer note is in [docs/VERSIONS.md](docs/VERSIONS.md).

## How v2 watches

1. The camera task grabs an RGB565 frame (QVGA by default) and keeps a coarse luma image, about 80×60.
2. Pixels that changed since the previous frame are grouped into a 10×8 grid. The strongest cluster becomes one target. A change smaller than 2% of the frame is noise. A change larger than 40% of the frame is treated as too close and dropped.
3. The eye task smooths that point, holds it for about 1.5 s after it disappears, then lets the pupils drift home and glance around. The pupils also blink.
4. The display task draws two eyes into a circular dial on the 240×280 panel and flushes the eye band over SPI.

There is no face detector, no radar, and no wrist IMU in this version. A person who stops moving is held for the linger time and then released. A later tracker can replace the motion step without changing the eye drawing. Thresholds live in `firmware/main/config.h`. The face itself is `k_face` in `firmware/main/face.h`.

The panel is a rounded rectangle, not a round GC9A01. The firmware paints a black circular mask (diameter 232, centered on the glass) so the eyes sit in a watch-like window. Details are in [hardware/v2-dnesp32s3-camera/README.md](hardware/v2-dnesp32s3-camera/README.md).

## Hardware

| Part | Role |
| --- | --- |
| [正点原子 DNESP32S3](https://www.alientek.com/Product_Details_12/28.html) | ESP32-S3, USB, XL9555 expander, DVP camera socket, SPI LCD socket |
| Kit OV5640 | Plugs into connector P2. SCCB and 8-bit DVP. Reset and power-down are on the XL9555 |
| [Waveshare 1.69″ Touch LCD](https://www.waveshare.com/wiki/1.69inch_Touch_LCD_Module) | ST7789V2 over SPI, 240×280. CST816T touch is on the module and is not read yet |

Wiring, the P5 `IO_SET`–`LCD_DC` jumper, and the pin table are in [hardware/v2-dnesp32s3-camera/README.md](hardware/v2-dnesp32s3-camera/README.md). v1 drawings stay on the v1 branch; [hardware/v1-radar-amoled/README.md](hardware/v1-radar-amoled/README.md) is only a pointer.

## Firmware

ESP-IDF and FreeRTOS, target `esp32s3`. Three tasks:

| Task | Core | Work |
| --- | ---: | --- |
| `cam` | 0 | Capture, or a synthetic walker when `SIMULATE_CAMERA` is 1 |
| `eye` | 0 | Pick the look point, smooth it, blink |
| `disp` | 1 | Draw the face and blit it to the ST7789 |

Install and flash steps are in [firmware/README.md](firmware/README.md). The v1 firmware is not copied into this tree. [firmware-v1-radar/README.md](firmware-v1-radar/README.md) says where it lives.

```bash
cd firmware
idf.py set-target esp32s3
idf.py build
idf.py -p PORT flash monitor
```

`SIMULATE_CAMERA` defaults to 0, so a normal build opens the OV5640. Set it to 1 for a first look at the eyes with no sensor.

## Layout

```text
.
├── firmware/                         v2 ESP-IDF project (the default build)
│   ├── main/
│   ├── test/                         host test for motion and gaze
│   └── README.md
├── firmware-v1-radar/                pointer to the v1 firmware branch
├── hardware/
│   ├── v1-radar-amoled/              pointer to the v1 schematic and PCB
│   └── v2-dnesp32s3-camera/          v2 wiring
├── docs/VERSIONS.md
├── LICENSE
└── README.md
```

## License

[MIT](LICENSE).
