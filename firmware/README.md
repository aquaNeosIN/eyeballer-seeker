# v2 firmware

ESP-IDF project for the ESP32-S3 on a 正点原子 DNESP32S3. It draws a face on a Waveshare 1.69″ ST7789V2 and aims the eyes from an OV5640.

The radar firmware is a different project. See [firmware-v1-radar/README.md](../firmware-v1-radar/README.md).

## What is implemented

| Piece | Behavior |
| --- | --- |
| Capture | `esp32-camera` 2.1.8, DVP, RGB565, QVGA (320×240) in PSRAM, two frame buffers, grab-latest. XCLK is left unconnected because the kit module has a 24 MHz oscillator |
| Target | Frame difference on a luma image of about 80×60. A 10×8 grid. Center of mass of the strongest cluster and its neighbors. One target |
| Reject | Under 2% of the frame: ignore. Over 40% of the frame: too close, ignore. Stale samples older than 300 ms: ignore |
| Gaze | Exponential smooth (about 90 ms while tracking, about 480 ms while idle). Hold the last look for 1.5 s, then drift home and occasionally glance. Blink about every 3 s |
| Face | Two eyes, sclera, iris, pupil, highlight, inside a circular mask on the 240×280 panel. Edit `k_face` in `main/face.h` |
| Display | `esp_lcd` ST7789 driver, SPI2, 20 MHz, RGB565. Two full-frame buffers in PSRAM when the alloc succeeds, otherwise one buffer |
| Touch | Not read |
| IMU / radar | Not present |

A person who stands still becomes "no motion" and the eyes release them after the linger. Replacing `track_motion()` is the place for a future face or person detector. The eye task only consumes a `sighting_t` (x, y, area).

`SIMULATE_CAMERA` defaults to **0**. The sensor is opened. Set it to 1 to publish a walker that sweeps left and right with a gap at each end, which is enough to see the linger without a camera:

```c
#define SIMULATE_CAMERA 1
```

The macro is wrapped in `#ifndef`, so a build can also add this after `idf_component_register` in `main/CMakeLists.txt`:

```cmake
target_compile_definitions(${COMPONENT_LIB} PRIVATE SIMULATE_CAMERA=1)
```

## Prepare

- Board: 正点原子 DNESP32S3, with the kit OV5640 on P2 and the 1.69″ module wired as in [hardware/v2-dnesp32s3-camera/README.md](../hardware/v2-dnesp32s3-camera/README.md).
- USB cable to the board's native USB/JTAG connector. Logs use that port (`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG`). The separate UART0 header is not the console.
- ESP-IDF **v5.5.5** (the version this tree was built with). v5.4 or newer satisfies `idf_component.yml`. The panel driver is the ST7789 built into `esp_lcd`. The camera comes from the component registry (`espressif/esp32-camera` 2.1.8).

Install, with the ESP32-S3 toolchain selected:

https://docs.espressif.com/projects/esp-idf/en/v5.5.5/esp32s3/get-started/index.html

Each new shell needs the export script:

```bash
. $HOME/esp/esp-idf/export.sh
```

## Build and flash

```bash
cd firmware
idf.py set-target esp32s3
idf.py build
idf.py -p PORT flash monitor
```

`sdkconfig.defaults` selects the chip, 16 MB QIO flash, 80 MHz octal PSRAM, a 240 MHz CPU, USB logs, and a 3 MB factory app. That matches an ESP32-S3-WROOM-1-N16R8, which is the module used on the public DNESP32S3 ports this pin map was checked against. `idf.py set-target esp32s3` is still required once, so the toolchain and `sdkconfig` exist.

`PORT` is the serial device the USB port enumerates. On Linux that is often `/dev/ttyACM0`. Leave the monitor with `Ctrl+]`.

If the chip ignores the download, hold BOOT, tap RESET, release BOOT, and run `flash` again.

## Tasks

Created in `app_main`.

| Task | Core | Period | Work |
| --- | ---: | ---: | --- |
| `cam` | 0 | 50 ms | `camera_feed_poll()` into a depth-1 queue |
| `eye` | 0 | 33 ms | Smooth, blink, publish the look |
| `disp` | 1 | on each look | Draw the eye band and blit it |

Queues are one slot deep. A new sample overwrites a sample that has not been read. The display task blocks on the previous SPI transfer before it reuses a buffer.

The log line from `eye` is `track`, `linger`, or `idle`, then the sighting in hundredths (`x`, `y`), the area in thousandths, and the smoothed look. `close` means the frame was rejected for covering too much of the image. `display fps=` is the blit rate of the eye band.

On the sensor log, `pid=0x5640` is an OV5640. Another PID means a different camera answered on SCCB; the driver may still run, and the log says it is not the kit part.

## Buffers

A 240×280 RGB565 frame is 134400 bytes. The display task asks PSRAM for two of them, 64-byte aligned, so SPI can DMA from PSRAM. If the second alloc fails it keeps one and logs `single framebuffer`. If both fail, the display task stops drawing and the other tasks keep logging.

The camera keeps two QVGA RGB565 frames in PSRAM (about 150 KB each). The motion pass keeps two luma buffers in internal RAM, capped at 160×120 (38400 bytes together).

ESP-IDF v5.5.5, `idf.py size` on this tree:

| Item | Size |
| --- | --- |
| Application image | 350097 bytes (`eyeballer-seeker.bin` is 0x55800, padded) |
| 3 MB app partition free | 0x2aa800, about 89% |
| Internal RAM (DIRAM) | 128655 used, 213105 free, of 341760 |
| IRAM | 16384 / 16384. The startup code fills this |
| Flash code | 177842 bytes |
| Flash read-only data | 70040 bytes |

The two screen buffers are allocated at runtime, so they are not in that table.

`sdkconfig` selects QIO flash. `CONFIG_ESPTOOLPY_FLASH_MODE_AUTO_DETECT` stays on, so the `idf.py flash` command line prints `--flash_mode dio`. That is the ROM header mode; the QIO choice is still the one in `sdkconfig.defaults`.

## Host test

Motion and gaze can be checked without a board or an IDF install:

```bash
gcc -std=c11 -Wall -Wextra -Werror -I firmware/main -o /tmp/test_logic \
  firmware/test/test_logic.c firmware/main/track.c firmware/main/gaze.c -lm
/tmp/test_logic
```

`logic ok` means: a blob on the right is selected, a full-frame change is dropped, a two-pixel speck is ignored, and the gaze lingers and then releases.

## If the picture is wrong

| What you see | Change in `main/config.h` |
| --- | --- |
| Shifted down or up by a band | `LCD_ROW_OFFSET` (default 20) |
| Negative image | `LCD_INVERT_COLOR` |
| Red and blue swapped | `LCD_BGR` |
| Upside down or mirrored | `LCD_MIRROR_X`, `LCD_MIRROR_Y` |
| Camera image feels backwards, so the eyes look the wrong way | `CAM_HMIRROR`, `CAM_VFLIP` |

SPI is 20 MHz so Dupont wires have some margin. `LCD_SPI_HZ` can be raised after the geometry is right.

## Boot assumptions that show up as a crash

- Octal PSRAM is turned on. A board with quad PSRAM, or with none, will not finish boot. The setting is documented in the v2 wiring notes.
- The image is built for 16 MB of flash. A smaller module needs `CONFIG_ESPTOOLPY_FLASHSIZE_*` changed before flashing, or esptool will refuse the image.
