[English](README.md) | [简体中文](README.zh-CN.md)

# 固件

本目录是「手持热成像仪」的 ESP-IDF 固件源码。

- 项目主页：[根 README](../README.md)
- 装配说明：[docs/assembly.md](../docs/assembly.md)
- 硬件设计：[hardware/](../hardware/)
- 外壳模型：[enclosure/](../enclosure/)

## 1. 支持的硬件配置

| 项目 | 选项 |
|---|---|
| 主控 | ESP32-S3 |
| 屏幕 | IPS / TFT |
| 测距模块 | 安装 TOF400C / 不安装 |
| 热成像传感器 | MLX90640 |
| 激光 | 6mm 激光管，辅助定位 |
| 存储 | Micro SD Card，可选，用于截图 |
| 供电 | 18650 电池，USB 5V 充电 |

对应四种固件变体：

| 固件 | 屏幕 | TOF400C |
|---|---|---|
| `firmware_tof_ips.bin` | IPS | 安装 |
| `firmware_no_tof_ips.bin` | IPS | 不安装 |
| `firmware_tof_tft.bin` | TFT | 安装 |
| `firmware_no_tof_tft.bin` | TFT | 不安装 |

## 2. 开发环境

| 项目 | 版本 |
|---|---|
| ESP-IDF | v5.5.4 |
| Target | esp32s3 |
| Python | 3.10+ |
| 构建系统 | CMake + Ninja |

请先安装 ESP-IDF v5.5.4，并执行 `export.sh` / `export.bat` 激活环境。

```bash
# Linux / macOS
. $IDF_PATH/export.sh

# Windows
%IDF_PATH%\export.bat
```

> 不建议使用与 v5.5.4 差异较大的 IDF 版本，API 可能不兼容。

## 3. 快速开始

```bash
git clone https://github.com/你的用户名/你的仓库.git
cd 你的仓库/firmware

idf.py set-target esp32s3
idf.py build
idf.py -p <PORT> flash monitor
```

Windows 端口示例：`COM4`  
Linux/macOS 端口示例：`/dev/ttyUSB0`、`/dev/ttyACM0`

退出 monitor：`Ctrl + ]`

## 4. 固件变体怎么构建

本项目通过 config.h [../firmware/main/config.h](../firmware/main/config.h) 宏定义区分屏幕和 TOF。  
请根据实际硬件选择配置，不要烧错固件。

- 示例：构建 IPS + TOF 版本

在 config.h 中 取消注释
#define VL53L1X 和 #define IPS_screen

```bash
idf.py set-target esp32s3
idf.py build
```

- 示例：构建 TFT + NoTOF 版本

在 config.h 中 注释
//#define VL53L1X 和 //#define IPS_screen

```bash
idf.py fullclean
idf.py set-target esp32s3
idf.py build
```

构建完成后，合并固件：

```bash
idf.py merge-bin -o build/firmware_tof_ips.bin
```

## 5. 目录结构

```text
firmware/
├── CMakeLists.txt
├── sdkconfig.defaults
├── partitions.csv
├── main/                  # 主程序入口、任务调度、初始化
├── components/
│   ├── buffer/            # 帧缓冲、图像缓存
│   ├── display/           # IPS/TFT 屏幕驱动、绘图接口
│   ├── mlx90640/          # MLX90640 热成像传感器驱动
│   ├── peripherals/       # 按键、电池、SD 卡、激光等外设
│   ├── task/              # FreeRTOS 任务创建与调度
│   ├── ui/                # 界面、菜单、色彩方案
│   └── vl53l1/            # VL53L1 / TOF400C 测距驱动
└── tools/                 # 辅助脚本
```

各组件职责：

| 组件 | 职责 | 依赖 |
|---|---|---|
| `buffer` | 图像帧缓存、双缓冲 | - |
| `display` | 屏幕初始化、像素写入、刷新 | `buffer` |
| `mlx90640` | I2C 读取热成像数据、温度换算 | `peripherals` |
| `peripherals` | 按键、电池 ADC、SD、激光 GPIO | - |
| `task` | 采集、处理、显示、UI 任务 | 以上组件 |
| `ui` | 菜单、色彩方案、温度单位、热点冷点 | `display`、`buffer` |
| `vl53l1` | 激光测距，实验性 | `peripherals` |

## 6. 配置项

本项目使用 `sdkconfig.defaults` 保存非默认配置，包括 PSRAM、Flash、主任务栈大小等。
`idf.py` 在首次构建或 `sdkconfig` 不存在时会自动用它生成 `sdkconfig`。

## 7. 引脚定义

| 功能 | 信号 | GPIO | 备注 |
|---|---|---|---|
| MLX90640 | SDA | GPIO17 | I2C0 |
| MLX90640 | SCL | GPIO18 | I2C0 |
| 屏幕 | MOSI | GPIO11 | SPI0 |
| 屏幕 | SCLK | GPIO12 | SPI0 |
| 屏幕 | CS | GPIO10 | SPI0 |
| 屏幕 | DC | GPIO9 | SPI0 |
| 屏幕 | RST | GPIO13 | - |
| 按键 | KEY_UP | GPIO40 | 上 |
| 按键 | KEY_DOWN | GPIO41 | 下 |
| 按键 | KEY_TRIGGER | GPIO42 | 前 |
| 按键 | KEY_RIGHT | GPIO39 | 右 |
| 激光 | LASER | GPIO5 | 辅助定位 |
| TOF400C | SDA | GPIO19 | I2C1 |
| TOF400C | SCL | GPIO20 | I2C1 |
| SD Card | MOSI | GPIO21 | SPI1 |
| SD Card | SCLK | GPIO47 | SPI1 |
| SD Card | CS | GPIO14 | SPI1 |
| SD Card | MISO | GPIO48 | SPI1 |
| SD Card | CD | GPIO38 | - |
| 蜂鸣器 | BUZ | GPIO7 | PWM输出 |
| 电池 ADC | ADC | GPIO6 | 分压采样 |

## 8. 烧录

### 8.1 开发烧录

```bash
idf.py -p <PORT> flash monitor
```

### 8.2 最终用户烧录

Release 中提供的是**合并固件**，起始地址 `0x0`：

```text
firmware_tof_ips.bin
firmware_no_tof_ips.bin
firmware_tof_tft.bin
firmware_no_tof_tft.bin
```

使用 `flash_download_tool` 或 `esptool.py`：

```bash
esptool.py --chip esp32s3 -p <PORT> -b 115200 write_flash 0x0 firmware_tof_ips.bin
```

> 只有合并固件才能从 `0x0` 烧录。自行编译的 app-only bin 需要按分区地址分别烧录：bootloader `0x0`、partition-table `0x8000`、app `0x10000`。

## 9. 调试

### 9.1 色彩方案调试

如果需要对色彩方案进行调整，可使用色彩方案调试器[color_scheme_adjuster.html](../firmware/tools/color_scheme_adjuster.html)：

- 使用浏览器打开此脚本，在界面左上角选择要进行调整的方案。
- 通过改变 T1 、T2 ...参数的大小来调整色域占比。
- 调整完毕后点击界面下方的 `导出全部 6 方案`，脚本将自动生成修改后的色彩方案数据表表并复制到剪切板上。
- 将剪切板上的信息粘贴到 buffer.c [components/buffer/buffer.c](components/buffer/buffer.c) 中的 static const uint16_t g_color_luts[COLOR_SCHEME_COUNT][256] = { ... } 内。
- 重新编译并烧录固件。

### 9.2 最终调试

- 串口日志：`idf.py monitor`，波特率 115200。
- 日志等级：`idf.py menuconfig` → `Component config` → `Log output`。
- OpenOCD / JTAG：`.vscode/settings.json` 中 `idf.openOcdConfigs` 使用 `board/esp32s3-builtin.cfg`。
- 如果无法烧录：按住 BOOT，再按 RESET，进入下载模式。
- 如果屏幕花屏：检查 SPI 频率、排线方向、屏幕型号是否与固件匹配。
- 如果 MLX90640 无数据：检查 I2C 地址、上拉电阻、供电。

## 10. 已知问题

- TOF400C 测距功能为实验性，最大有效距离约 400 cm，执行测距成功率不稳定。
- IPS / TFT 固件不通用，烧录错误会导致屏幕显示颜色反转。
- 安装 TOF400C 必须使用 PCB1，不安装则使用 PCB2。
- 截图功能依赖 Micro SD Card，未插卡时不可用。

## 11. 许可证

固件采用 PolyForm Noncommercial 1.0.0，见 [LICENSE.md](LICENSE.md)。

硬件设计、文档的许可证见根目录 [README](../README.md)。