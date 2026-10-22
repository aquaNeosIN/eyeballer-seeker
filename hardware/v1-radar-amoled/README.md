# v1 — radar and round AMOLED

This folder is a pointer. The schematic, the carrier PCB, and the block diagram are on the v1 branch so they are not copied into the camera tree.

| | |
| --- | --- |
| Pull request | https://github.com/aquaNeosIN/eyeballer-seeker/pull/1 |
| Branch | `cursor/hardware-schematic-2838` |
| Board | Waveshare ESP32-S3 Touch AMOLED 1.75 |
| Display | CO5300, QSPI, about 466×466 |
| Sensor | HLK-LD2450, plus the board's QMI8658 |
| Extra power | TPS61023 (or a fixed 5V module) from the battery, for the radar only |

On that branch:

| File | What it is |
| --- | --- |
| `hardware/README.md` | Wiring, current estimate, checks before soldering |
| `hardware/schematic.png` | Schematic drawing |
| `hardware/schematic.pdf` | Same drawing, for printing |
| `hardware/block-diagram.png` | Battery, boost, radar, display |
| `hardware/pcb-top.png`, `hardware/pcb-bottom.png` | Carrier PCB |
| `hardware/eyeballer-seeker.kicad_pro` | KiCad 7 or 8 project |

Browse them on GitHub:

https://github.com/aquaNeosIN/eyeballer-seeker/tree/cursor/hardware-schematic-2838/hardware

Or check the branch out locally:

```bash
git fetch origin cursor/hardware-schematic-2838
git switch cursor/hardware-schematic-2838
```

Do not copy that `hardware/` directory onto this v2 branch over `hardware/v2-dnesp32s3-camera/`. The two folders describe different boards.
