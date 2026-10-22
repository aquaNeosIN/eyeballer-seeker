# Versions

eyeballer-seeker has two hardware stacks. They are kept on separate branches so a radar schematic and a camera prototype are not mixed into one tree.

## v1 — radar and round AMOLED

| | |
| --- | --- |
| Pull request | https://github.com/aquaNeosIN/eyeballer-seeker/pull/1 |
| Branch | `cursor/hardware-schematic-2838` |
| Board | Waveshare ESP32-S3 Touch AMOLED 1.75, standard or `-B` |
| Display | CO5300, QSPI, about 466×466, round |
| Sensing | HLK-LD2450 radar on UART1 (GPIO17 / GPIO18, 256000 8N1) |
| Motion gate | QMI8658 IMU. Eyes stop following while the wrist is moving |
| Power | A 5V boost from the battery for the radar. The carrier PCB is in that branch |
| Firmware path on that branch | `firmware/` |
| Drawings on that branch | `hardware/schematic.png`, `hardware/schematic.pdf`, `hardware/eyeballer-seeker.kicad_pro` |

To build v1, check out that branch and follow its `firmware/README.md`. It was built with ESP-IDF v5.5.5. This v2 tree does not contain those sources or the KiCad files. Copying them here would duplicate the schematics and the board drawings, and the v1 pull request would stop being the place those files are edited.

```bash
git fetch origin cursor/hardware-schematic-2838
git switch cursor/hardware-schematic-2838
```

## v2 — camera, DNESP32S3, 1.69″ SPI

| | |
| --- | --- |
| This tree | `firmware/` and `hardware/v2-dnesp32s3-camera/` |
| Board | 正点原子 DNESP32S3 |
| Display | Waveshare 1.69″ Touch LCD, 240×280, ST7789V2, 4-wire SPI |
| Sensing | Kit OV5640 on connector P2. Motion and center of mass |
| Not used | LD2450, 5V boost, QMI8658, CO5300, the v1 carrier PCB |
| Touch | CST816T is on the module. The firmware does not read it |

v2 reuses the v1 idea of a smoothed pupil, a blink, a short linger, and an idle glance. The radar parser, the IMU, the power-management probe, and the QSPI panel driver are not part of this build.

## Which one to flash

Flash v1 only onto the Waveshare AMOLED board. Flash v2 only onto the DNESP32S3. The GPIO maps are not interchangeable: v1's display clock is GPIO38, which v2 uses as the camera SCCB clock.
