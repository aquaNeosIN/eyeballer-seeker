# v1 firmware

The radar firmware is not in this tree. It lives on the v1 branch at `firmware/`, next to the schematic it was written for.

| | |
| --- | --- |
| Pull request | https://github.com/aquaNeosIN/eyeballer-seeker/pull/1 |
| Branch | `cursor/hardware-schematic-2838` |
| Path | `firmware/` |
| Target | ESP32-S3 on the Waveshare ESP32-S3 Touch AMOLED 1.75 |
| Display | `espressif/esp_lcd_co5300`, QSPI, 466×466 |
| Tasks | `radar`, `imu`, `eye`, `disp` |
| Default | `SIMULATE_RADAR` is 1, so the eyes sweep without the module |

That project was built with ESP-IDF v5.5.5. Its own `firmware/README.md` has the flash steps and the GPIO table (LCD D0–D3 on GPIO4–GPIO7, SCLK GPIO38, CS GPIO12, RST GPIO39, I2C GPIO15/GPIO14, radar UART on GPIO17/GPIO18).

```bash
git fetch origin cursor/hardware-schematic-2838
git switch cursor/hardware-schematic-2838
cd firmware
idf.py set-target esp32s3
idf.py build
```

The v2 project in `/firmware` of this branch is a different build. It will not drive the round AMOLED panel, and the v1 project will not drive the DNESP32S3 camera.
