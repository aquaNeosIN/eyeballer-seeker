# v2 wiring — DNESP32S3, OV5640, 1.69″ SPI

Breadboard or Dupont wires. No new PCB.

```text
  kit OV5640
      │  seated in P2 (DVP + SCCB)
      ▼
  DNESP32S3 ──── SPI ──── Waveshare 1.69″ ST7789V2
      │                   CST816T left unconnected
      USB (log and flash)
```

The watch face is the 1.69″ module, turned outward. The camera looks the same way, out through P2, so the wearer is behind the lens.

## Parts

| Part | Notes |
| --- | --- |
| 正点原子 DNESP32S3 | ESP32-S3 development board. The kit camera socket is P2. The SPI LCD socket is the 2×4 SPILCD header |
| OV5640 module from the same kit | 8-bit DVP, SCCB, onboard 24 MHz oscillator. Sits on P2. Do not flywire the data bus |
| Waveshare 1.69″ Touch LCD Module | 240×280 IPS, ST7789V2, 4-wire SPI. Rounded rectangle, not a round GC9A01. [Wiki](https://www.waveshare.com/wiki/1.69inch_Touch_LCD_Module), [product page](https://www.waveshare.com/product/1.69inch-touch-lcd-module.htm) |

Power the display from the board's **3.3 V**, not 5 V. The wiki says the supply voltage and the logic voltage have to match. The camera takes 3.3 V from P2.

Leave the microSD card out. Its clock and MOSI are the same GPIO11 and GPIO12 as the LCD.

## Display wires

Match signals. The SPILCD header is 2×4 and the silkscreen is the pin map; this note does not guess a pin number from a photo. SPI signals can also be taken from the GPIO headers. Reset and backlight cannot: on this board they are XL9555 pins, brought out on the SPILCD socket.

| Waveshare pin | Connect to | ESP32 / expander |
| --- | --- | --- |
| VCC | 3.3 V | — |
| GND | GND | — |
| LCD_DIN | SPI MOSI | GPIO11 |
| LCD_CLK | SPI SCK | GPIO12 |
| LCD_CS | chip select, active low | GPIO21 |
| LCD_DC | data / command | GPIO40 |
| LCD_RST | SPILCD reset net | XL9555 IO1_2, active low |
| LCD_BL | SPILCD PWR / backlight net | XL9555 IO1_3, active high |
| TP_SDA, TP_SCL, TP_RST, TP_IRQ | leave open | touch is not read |

SPI mode is 0 (CPOL 0, CPHA 0), MSB first, 20 MHz. There is no MISO wire.

### Jumper

The Alientek IDF guide says: on header P5, fit a jumper between **IO_SET** and **LCD_DC**. A board port of the same PCB calls those pins IO_SEL and LCD_DC, P5 pins 4–6. Fit the jumper when LCD_DC is taken from the SPILCD socket, so GPIO40 actually reaches that pin. A Dupont wire landed on the GPIO40 pad itself does not need the jumper. Fitting it either way is the setup the Alientek LCD chapters use.

### If the socket nets are hard to reach

`firmware/main/config.h` can move reset and backlight onto GPIO pins:

```c
#define LCD_USE_XL9555 0
```

Then LCD_RST is GPIO8 and LCD_BL is GPIO43. GPIO43 is also UART0 TX on the spare serial header, so leave that header empty. GPIO8 is not used by the camera, the codec, the SD socket, or the octal PSRAM. The camera reset pins stay on the XL9555 either way, because P2 is wired that way on the board.

### Picture geometry

The ST7789V2 framebuffer is 240×320. The glass is 240×280, so the firmware sets a **row offset of 20** and a column offset of 0. That is the usual 240×280 window (TFT_eSPI's 240×280 setup, and the gap described for this panel). Color inversion is on, which this IPS module wants. If the image is shifted, change `LCD_ROW_OFFSET`. If it is a negative, set `LCD_INVERT_COLOR` to 0. If red and blue are swapped, set `LCD_BGR` to 1. Mirrors are `LCD_MIRROR_X` and `LCD_MIRROR_Y`.

The firmware draws the eyes inside a circle of diameter 232 centered at (120, 140). Everything outside that circle is black. The module's rounded corners already hide part of the rectangle; the circle is a software mask so the face reads as a watch. It does not turn the panel into a round controller.

## Camera

Seat the kit OV5640 on **P2**. The firmware uses the same pins as the Alientek camera chapter (IDF guide V1.6) and the same map published in the xiaozhi `atk-dnesp32s3` board file.

| Signal | GPIO or expander |
| --- | --- |
| SCCB SCL | GPIO38 |
| SCCB SDA | GPIO39 |
| VSYNC | GPIO47 |
| HREF | GPIO48 |
| PCLK | GPIO45 |
| D0–D7 | GPIO4, 5, 6, 7, 15, 16, 17, 18 |
| RESET | XL9555 IO0_5, active low |
| PWDN | XL9555 IO0_4, active high (firmware drives it low) |
| XCLK | not connected. The module has its own 24 MHz oscillator |

GPIO45 is an ESP32-S3 strapping pin. The board routes the camera pixel clock there, and the Alientek camera example does the same. This firmware does not change the boot straps.

PWDN and RESET are released through the XL9555 before `esp_camera_init`. The expander is I2C address **0x20** (A2, A1, A0 grounded), SDA GPIO41, SCL GPIO42, 400 kHz. That bus is shared with the ES8388 (address 0x10) and the onboard 24C02. This firmware only talks to 0x20.

The beeper is XL9555 IO0_3. In the guide's EXIO example, writing 0 turns it on and writing 1 turns it off, so the firmware leaves that bit high.

## What the expander pins are

From the same guide, chapter 19 (IIC EXIO) and the camera chapter:

| XL9555 pin | Bit | Direction | Use |
| --- | ---: | --- | --- |
| IO0_3 | 3 | output | beeper, active low |
| IO0_4 | 4 | output | camera PWDN |
| IO0_5 | 5 | output | camera RESET |
| IO1_2 | 10 | output | SPI LCD reset |
| IO1_3 | 11 | output | SPI LCD backlight |
| IO1_4 … IO1_7 | 12 … 15 | input | KEY3 … KEY0, not read |

Direction registers written at boot: port 0 = `0x03` (IO0_0 and IO0_1 stay inputs), port 1 = `0xF0` (keys stay inputs, LCD pins are outputs).

## Pins this firmware does not take

| Function | GPIO | Why it is left alone |
| --- | --- | --- |
| USB D− / D+ | 19 / 20 | Flash and log |
| ES8388 I2S | MCLK 3, WS 9, DOUT 10, DIN 14, BCLK 46 | Codec is on the board |
| SD card | CS 2, SCK 12, MOSI 11, MISO 13 | SCK and MOSI are shared with the LCD. MISO is unused here. Card stays out |
| Octal PSRAM | 33–37, inside the module | Not available as GPIO |
| Quad flash | 26–32, inside the module | Not available as GPIO |

## Assumptions

These are the choices the firmware is built around. They come from the public Alientek text and from board ports that name this PCB, not from a meter on one particular board.

1. **Module.** `sdkconfig.defaults` targets an ESP32-S3-WROOM-1-N16R8: 16 MB quad flash (QIO, 80 MHz) and 8 MB octal PSRAM at 80 MHz. That is the module named by the xiaozhi `atk-dnesp32s3` port and by [mirui-atk-dnesp32s3](https://github.com/tricorelife/mirui-atk-dnesp32s3). A different module needs a different flash size and PSRAM mode. If the ROM log says the PSRAM was not found, the octal setting does not match the chip.
2. **Camera and SPI pins.** Alientek 《DNESP32S3使用指南—IDF版》V1.6, camera chapters, as republished (for example [chapter 35](https://www.cnblogs.com/zdyz/p/19052430)). The same numbers are in `main/boards/atk-dnesp32s3/config.h` of [78/xiaozhi-esp32](https://github.com/78/xiaozhi-esp32).
3. **XL9555 address and beeper polarity.** Guide chapter 19: address `0x20`, beeper on with 0 and off with 1. [Blog mirror](https://www.cnblogs.com/zdyz/p/18851916).
4. **Row offset 20, invert on.** Waveshare describes the controller as 240×320 RAM behind a 240×280 glass. TFT_eSPI's 240×280 rotation table uses column 0 and row 20 in portrait. Inversion is the usual IPS ST7789 setting and is a one-line change if the photo looks wrong.
5. **Backlight polarity.** The guide names IO1_3 "PWR" and the EXIO header calls it the SPI LCD backlight. The firmware drives it high to light the panel, matching the Alientek snippets that call `LCD_PWR(1)` after init. The Waveshare `LCD_BL` examples also drive the pin high for on.
6. **Touch chip name.** The product page says CST816T. Parts of the wiki say CST816S or CST816D. The 7-bit address used by those chips is `0x15`. Nothing in this firmware reads it.
7. **No depth.** v1 dropped radar returns inside 0.5 m so the wearer was ignored. v2 has no range. A motion blob that covers more than 40% of the sampled frame is dropped instead. The camera is assumed to face outward with the display.

## Sources

- Alientek DNESP32S3 product page: https://www.alientek.com/Product_Details_12/28.html
- Camera pin list (guide V1.6, chapter 35): https://www.cnblogs.com/zdyz/p/19052430
- XL9555 address `0x20` and the beeper example (chapter 19): https://www.cnblogs.com/zdyz/p/18851916
- Waveshare 1.69″ Touch LCD: https://www.waveshare.com/wiki/1.69inch_Touch_LCD_Module
- Board-level pin confirmation from a public port: https://github.com/78/xiaozhi-esp32/blob/417f52d7/main/boards/atk-dnesp32s3/config.h
