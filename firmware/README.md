[English](README.md) | [简体中文](README.zh-CN.md)

# Firmware

This directory contains the ESP-IDF firmware source code for the "Handheld Thermal Imager".

- Project home: [Root README](../README.md)
- Assembly instructions: [docs/assembly.md](../docs/assembly.md)
- Hardware design: [hardware/](../hardware/)
- Enclosure models: [enclosure/](../enclosure/)

## 1. Supported Hardware Configurations

| Item | Options |
|---|---|
| MCU | ESP32-S3 |
| Display | IPS / TFT |
| Ranging module | TOF400C installed / not installed |
| Thermal sensor | MLX90640 |
| Laser | 6 mm laser module, for aiming assistance |
| Storage | Micro SD Card, optional, for screenshots |
| Power | 18650 battery, USB 5V charging |

Four firmware variants:

| Firmware | Display | TOF400C |
|---|---|---|
| `firmware_tof_ips.bin` | IPS | Installed |
| `firmware_no_tof_ips.bin` | IPS | Not installed |
| `firmware_tof_tft.bin` | TFT | Installed |
| `firmware_no_tof_tft.bin` | TFT | Not installed |

## 2. Development Environment

| Item | Version |
|---|---|
| ESP-IDF | v5.5.4 |
| Target | esp32s3 |
| Python | 3.10+ |
| Build system | CMake + Ninja |

Please install ESP-IDF v5.5.4 first, and run `export.sh` / `export.bat` to activate the environment.

```bash
# Linux / macOS
. $IDF_PATH/export.sh

# Windows
%IDF_PATH%\export.bat
```

> Using an IDF version significantly different from v5.5.4 is not recommended, as APIs may be incompatible

## 3. Quick Start

git clone https://github.com/your-username/your-repository.git
cd your-repository/firmware

idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash monitor

Windows port example: COM4
Linux/macOS port examples: /dev/ttyUSB0, /dev/ttyACM0

Exit monitor: `Ctrl + ]`

## 4. How to Build Firmware Variants

This project uses macros in config.h [../firmware/main/config.h](../firmware/main/config.h) to distinguish between display and TOF options.
Select the configuration according to your actual hardware, and do not flash the wrong firmware.

- Example: Build the IPS + TOF version

In config.h, uncomment
#define VL53L1X and #define IPS_screen

```bash
idf.py set-target esp32s3
idf.py build
```

- Example: Build the TFT + NoTOF version

In config.h, comment out
//#define VL53L1X and //#define IPS_screen

```bash
idf.py fullclean
idf.py set-target esp32s3
idf.py build
```

After building, merge the firmware:

```bash
idf.py merge-bin -o build/firmware_tof_ips.bin
```

## 5. Directory Structure

```text
firmware/
├── CMakeLists.txt
├── sdkconfig.defaults
├── partitions.csv
├── main/                  # Main program entry, task scheduling, initialization
├── components/
│   ├── buffer/            # Frame buffer, image cache
│   ├── display/           # IPS/TFT display driver, drawing interface
│   ├── mlx90640/          # MLX90640 thermal sensor driver
│   ├── peripherals/       # Buttons, battery, SD card, laser, and other peripherals
│   ├── task/              # FreeRTOS task creation and scheduling
│   ├── ui/                # UI, menus, color schemes
│   └── vl53l1/            # VL53L1 / TOF400C ranging driver
└── tools/                 # Helper scripts
```

Component responsibilities:
		
| Component | Responsibility | Dependencies |
|---|---|---|
| `buffer` | Image frame buffer, double buffering | - |
| `display` | Display initialization, pixel writing, refresh | `buffer` |
| `mlx90640` | I2C thermal data reading, temperature conversion | `peripherals` |
| `peripherals` | Buttons, battery ADC, SD, laser GPIO | - |
| `task` | Acquisition, processing, display, UI tasks | Components above |
| `ui` | Menus, color schemes, temperature unit, hot/cold spot | `display`、`buffer` |
| `vl53l1` | Laser ranging, experimental | `peripherals` |

## 6. Configuration

This project uses `sdkconfig.defaults` to store non-default configurations, including PSRAM, Flash, main task stack size, etc.
`idf.py` automatically uses it to generate `sdkconfig` on the first build or when `sdkconfig`does not exist.

## 7. 引脚定义

| Function | Signal | GPIO | Notes |
|---|---|---|---|
| MLX90640 | SDA | GPIO17 | I2C0 |
| MLX90640 | SCL | GPIO18 | I2C0 |
| Button | MOSI | GPIO11 | SPI0 |
| Button | SCLK | GPIO12 | SPI0 |
| Button | CS | GPIO10 | SPI0 |
| Button | DC | GPIO9 | SPI0 |
| Button | RST | GPIO13 | - |
| Button | KEY_UP | GPIO40 | UP |
| Button | KEY_DOWN | GPIO41 | DOWN |
| Button | KEY_TRIGGER | GPIO42 | Front |
| Button | KEY_RIGHT | GPIO39 | RIGHT |
| Laser | LASER | GPIO5 | Aiming assistance |
| TOF400C | SDA | GPIO19 | I2C1 |
| TOF400C | SCL | GPIO20 | I2C1 |
| SD Card | MOSI | GPIO21 | SPI1 |
| SD Card | SCLK | GPIO47 | SPI1 |
| SD Card | CS | GPIO14 | SPI1 |
| SD Card | MISO | GPIO48 | SPI1 |
| SD Card | CD | GPIO38 | - |
| Buzzer | BUZ | GPIO7 | PWM output |
| Battery ADC | ADC | GPIO6 | Voltage divider sampling |

## 8. Flashing

### 8.1 Development Flashing

```bash
idf.py -p <PORT> flash monitor
```

### 8.2 End-User Flashing

Release provides **merged firmware**, with start address `0x0`:

```text
firmware_tof_ips.bin
firmware_no_tof_ips.bin
firmware_tof_tft.bin
firmware_no_tof_tft.bin
```

Using `flash_download_tool` or `esptool.py`:

```bash
esptool.py --chip esp32s3 -p <PORT> -b 115200 write_flash 0x0 firmware_tof_ips.bin
```

> Only merged firmware can be flashed from `0x0`. A self-compiled app-only bin must be flashed separately according to partition addresses: bootloader `0x0`, partition-table `0x8000`, app `0x10000`.

## 9. Debugging

### 9.1 Color Scheme Debugging

If you need to adjust the color scheme, use the color scheme adjuster[color_scheme_adjuster.html](../firmware/tools/color_scheme_adjuster.html):
- Open this script in a browser and select the scheme to adjust in the upper-left corner.
- Adjust the T1, T2, ... parameters to change the color gamut proportions.
- After adjustment, click `导出全部 6 方案` at the bottom of the interface. The script will automatically generate the modified color scheme data table and copy it to the clipboard.
- Paste the clipboard content into static const uint16_t g_color_luts[COLOR_SCHEME_COUNT][256] = { ... } in buffer.c [components/buffer/buffer.c](components/buffer/buffer.c).
- Recompile and flash the firmware.

### 9.2 Final Debugging

- Serial logs: `idf.py monitor`, baud rate `115200`.
- Log level: `idf.py menuconfig` → `Component config` → `Log output`.
- OpenOCD / JTAG: use `board/esp32s3-builtin.cfg` for `idf.openOcdConfigs in` `.vscode/settings.json`.
- If flashing fails: hold BOOT, then press RESET to enter download mode.
- If the display shows garbled colors: check the SPI frequency, ribbon cable orientation, and whether the display model matches the firmware.
- If MLX90640 has no data: check the I2C address, pull-up resistors, and power supply.

## 10. Known Issues

- The TOF400C ranging function is experimental, with a maximum effective distance of about 400 cm, and ranging success rate is unstable.
- IPS / TFT firmware is not interchangeable; flashing the wrong one causes color inversion on the display.
- PCB1 must be used if TOF400C is installed; PCB2 must be used if it is not installed.
- The screenshot function depends on a Micro SD Card and is unavailable when no card is inserted.

## 11. License

- Hardware design (PCB, schematic): CC BY-NC-SA 4.0, see [LICENSE.md](LICENSE.md)
- Firmware source code: PolyForm Noncommercial 1.0.0, see [firmware/LICENSE.md](firmware/LICENSE.md)
- Documentation and images: CC BY-NC-SA 4.0, see [LICENSE.md](LICENSE.md)