[English](assembly.md) | [简体中文](assembly.zh-CN.md)

# Assembly Instructions

> Applicable hardware version: v0.1  
> PCB version: v0.1  
> Last updated: 2026-10-19  

## 1. Precautions

- Always check power supply polarity before powering on to prevent reverse connection;
- When using lithium batteries, do not short-circuit, overcharge, or puncture them;
- Release static electricity before touching the PCB to avoid damaging chips;
- All soldering, cable plugging/unplugging, and battery connection operations must be performed with power off;
- The temporary test battery must be disconnected before soldering the battery wires permanently;
- Laser safety: Do not look directly into the laser, do not point the laser at eyes or reflective surfaces, and keep children away;
- Because the ranging function has many bugs and the maximum effective distance of the laser ranging module is only 400 cm, users can choose whether to install the TOF400C laser ranging module;
- Two display options are available for this project (see links in the BOM);
- Two PCB/Gerber files are provided. If the user chooses to install the TOF400C laser ranging module, fabricate PCB1; otherwise, fabricate PCB2;
- The PCB consists of three parts that need to be manually separated during soldering and assembly. The largest part is main_board, followed by sensor_board, and the smallest is trigger_board;
- The PCB thickness must be 1.2 mm;
- It is recommended to separate the three PCBs first and smooth the edges with a file/sandpaper before soldering. If soldering before separation, be careful not to damage components or traces;
- Wear safety goggles when separating the panels to avoid flying debris;

## 2. PCB and BOM

- Gerber file 1: [hardware/gerber/Gerber_PCB1.zip](../hardware/gerber/Gerber_PCB1.zip)
- Gerber file 2: [hardware/gerber/Gerber_PCB2.zip](../hardware/gerber/Gerber_PCB2.zip)
- Schematic: [hardware/schematic/SCH_Schematic.pdf](../hardware/schematic/SCH_Schematic.pdf)
- Complete BOM: [hardware/bom/BOM_v0.1.csv](../hardware/bom/BOM_v0.1.csv) (viewable online) [hardware/bom/BOM_v0.1.xlsx](../hardware/bom/BOM_v0.1.xlsx) (editable source file)

It is recommended to go to OSHWHub for PCB fabrication:
- Link: https://oshwhub.com/big_smoke/project_bniojffv

## 3. Tools and Consumables

| Tool | Required | Notes |
|---|---|---|
| Soldering iron | Required | 300-350°C recommended |
| Solder | Required | 0.6-0.8 mm leaded or lead-free |
| Solder paste | Optional | Medium-temperature leaded or lead-free solder paste |
| Multimeter | Required | Continuity and voltage measurement |
| Screwdriver | Required | For screws |
| Super glue | Required | Fixing parts |
| Heating plate | Optional | SMD soldering |


## 4. Models and 3D Printing Parameters

- STL files: [enclosure/stl](../enclosure/stl)
- STEP files: [enclosure/step](../enclosure/step)
- 3MF files: [enclosure/3mf](../enclosure/3mf)

It is recommended to go to MakerWorld for 3D printing:
- MakerWorld International: https://makerworld.com/zh/models/3447109-handheld-thermal-imager-casing#profileId-3931800
- MakerWorld China: https://makerworld.com.cn/zh/models/3109405-shou-chi-re-cheng-xiang-yi-wai-ke#profileId-3661945

## 5. Assembly Steps

### 5.1 Recommended Soldering Steps

1. Solder the TP5400 power chip and its peripheral circuits on the main_board PCB, and solder the Type-C receptacle and its peripheral circuits;
2. Connect USB 5V to the Type-C port, and use a multimeter to measure whether the TP5400 output voltage is between 3.9 V and 5 V;
3. Temporarily connect a single-cell lithium battery to the H2 battery connector, with the positive terminal to VBAT and the negative terminal to GND (pay attention to polarity; do not reverse). Use a multimeter to measure whether the TP5400 output voltage is 5 V;
4. If the test results of steps 2 and 3 are normal, solder the SGM2205 chip and its peripheral circuits. After soldering, use a multimeter to measure whether the SGM2205 output voltage is 3.3 V;
5. If the test results of the first 4 steps are normal, continue soldering the other components;
6. Solder the components on the sensor_board PCB and trigger_board PCB. Do not solder the TOF400C module yet (if the user chooses to install it);

### 5.2 Enclosure Preparation Steps

1. 3D print all parts;
2. Remove the support material under overhangs on the main_body part, and use a file and sandpaper to smooth the support surfaces as much as possible;
3. If PCB1 was fabricated, assemble the top_1 part and top_2 part. If PCB2 was fabricated, assemble the top_1 part and top_2_no_tof part, and fix them with glue;

### 5.3 Final Assembly Steps

1.  Solder two wires (length > 50 mm) to the pads on the trigger_board PCB;
2.  Use M2 screws to fix the trigger_board PCB to the designated position on the main_body part;
3.  Assemble the trigger part and the main_body part;
4.  Insert the 18650 battery into the battery holder, then place the battery holder into the reserved position in the handle of the main_body part, and route the battery power wires through the inside of the main_body part;
5.  Assemble the bottom part with the main_body part;
6.  Solder the battery power wires to the H2 battery connector on the main_board PCB (pay attention to polarity; do not solder them in reverse);
7.  Solder the two wires from the trigger_board PCB to the H1 connector on the main_board PCB;
8.  Insert one end of the ZH1.5 7P ribbon cable into the female connector on the main_board PCB;
9.  Route the display ribbon cable through the notch on the main_board PCB and insert it into the FPC connector on the main_board PCB;
10. Use M2 screws to fix the main_board PCB to the main_body part;
11. Place the 6 mm laser module into the reserved position on the assembled top part;
12. If PCB1 was fabricated, place the TOF400C module into the reserved position on the top part. If PCB2 was fabricated, ignore this step;
13. Use M2 screws to fix the sensor_board PCB to the top part. If PCB1 is used, solder the TOF400C module pin header and the laser wires. If PCB2 is used, solder only the laser wires and ignore the TOF400C;
14. Insert the other end of the ZH1.5 7P ribbon cable into the connector on the sensor_board PCB, then assemble the top part with the main_body part;
15. Place the display into the retaining slot on the front part, and assemble the front part with the main_body part;
16. Install the four keycaps onto the switches. The keycap with the power symbol goes on the left, the keycap with the solid circle symbol goes on the right, the keycap with the upward arrow goes on the top, and the keycap with the downward arrow goes on the bottom;
17. If the keycaps are loose, apply glue as appropriate (do not let glue flow into the switch internals);

## 6. Firmware Flashing

1. Install the CH340 USB-to-serial driver;
2. Download and open flash_download_tool (https://dl.espressif.com/public/flash_download_tool.zip);
3. Select ESP32-S3, Develop, UART, OK;
4. Connect the device to the computer via a USB data cable and turn on the power switch of the device;
5. Select the firmware to flash from [firmware/bin](../firmware/bin): if the TOF400C module is installed and an IPS display is used, select firmware_tof_ips.bin; if the TOF400C module is not installed and an IPS display is used, select firmware_no_tof_ips.bin; if the TOF400C module is installed and a TFT display is used, select firmware_tof_tft.bin; if the TOF400C module is not installed and a TFT display is used, select firmware_no_tof_tft.bin;
6. Set the start address to 0x0, select the COM port, and set the baud rate to 115200, as shown in [docs/images/firmware_tools.png](../docs/images/firmware_tools.png);
 - The Release provides merged firmware, so the start address is 0x0.
 - If you compile it yourself, flash according to the ESP-IDF partition addresses: bootloader 0x0, partition-table 0x8000, app 0x10000.
7. Click START and wait for flashing to complete;

## 7. Power-On Test

1. Press the power switch and wait about 1 s;
2. Point the sensor at a heat source;
3. Check whether the screen displays a thermal image;

## 8. License

CC BY-NC-SA 4.0, see [../LICENSE.md](../LICENSE.md)