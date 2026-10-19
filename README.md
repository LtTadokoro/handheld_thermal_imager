[English](README.md) | [简体中文](README.zh-CN.md)

# Handheld Thermal Imager

This is an ESP32-S3-based handheld thermal imager that converts the heat distribution of a measured object into an image and displays it in real time.

![Product photo 1](docs/images/1.jpg)

![Product photo 2](docs/images/2.jpg)

![Product photo 3](docs/images/3.jpg)

![Product photo 4](docs/images/4.jpg)

![Product photo 5](docs/images/5.jpg)

For assembly instructions, see [docs/assembly.md](docs/assembly.md)

## Status
v0.1 prototype, basic functions verified.

## Features
- Switchable color schemes
- Physical button control panel
- Adjustable refresh rate
- Hot spot and cold spot display
- Laser ranging function (optional TOF400C, experimental, max about 400 cm, known issues)
- Screenshot function (requires Micro SD Card)
- Laser module for aiming assistance
- Adjustable temperature unit
- 18650 battery powered, 5V USB charging
- 3D-printable enclosure

## Directory
- `hardware/`: schematic, BOM, Gerber
- `firmware/`: firmware source code
- `enclosure/`: enclosure models
- `docs/`: assembly instructions, images

## Quick Start
> Note: PCB thickness must be 1.2 mm; whether to install TOF400C determines whether to fabricate PCB1 or PCB2; two display types, IPS/TFT, require different firmware.
1. Choose PCB1/PCB2 based on whether TOF400C is installed; PCB thickness must be 1.2 mm
2. Solder and complete bare-board power test
3. 3D print all parts
4. Assemble according to the assembly document
5. Flash firmware and perform final test

## Related Links
- OSHWHub PCB fabrication: (https://oshwhub.com/big_smoke/project_bniojffv)
- MakerWorld enclosure: (https://makerworld.com/zh/models/3447109-handheld-thermal-imager-casing#profileId-3931800) / (https://makerworld.com.cn/zh/models/3109405-shou-chi-re-cheng-xiang-yi-wai-ke#profileId-3661945)
- Assembly instructions: [docs/assembly.md](docs/assembly.md)
- Firmware: [firmware/bin](firmware/bin)

## Known Issues
- TOF400C ranging function is experimental, max about 400 cm, ranging success rate is unstable.

## License
- Hardware design (PCB, schematic): CC BY-NC-SA 4.0, see [LICENSE.md](LICENSE.md)
- Firmware source code: PolyForm Noncommercial 1.0.0, see [firmware/LICENSE.md](firmware/LICENSE.md)
- Documentation and images: CC BY-NC-SA 4.0, see [LICENSE.md](LICENSE.md)