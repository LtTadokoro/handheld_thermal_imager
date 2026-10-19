[English](assembly.md) | [简体中文](assembly.zh-CN.md)

# 装配说明

> 适用硬件版本：v0.1  
> PCB 版本：v0.1  
> 最后更新：2026-10-19  

## 1. 注意事项

- 上电前务必检查电源正负极，防止反接；
- 使用锂电池时，不要短路、过充、刺穿；
- 接触 PCB 前先释放静电，避免损坏芯片；
- 所有焊接、插拔排线、连接电池操作必须在断电状态下进行；
- 临时测试电池在正式焊接电池线前必须断开；
- 激光安全：不要直视激光，不要将激光指向人眼或反射面，儿童远离；
- 由于测距功能的bug较多，且激光测距模块的最大有效距离只有 400 cm，因此由用户自行选择是否安装 TOF400C 激光测距模块；
- 本项目存在两种屏幕可选（详见 BOM 中的链接）；
- 本项目存在两个 PCB/Gerber 文件，如果用户选择安装 TOF400C 激光测距模块，则打样 PCB1，反之则打样 PCB2 ；
- PCB 由三个部分组成，焊接、组装时需要手动掰开，最大的部分是 main_board ，其次是 sensor_board 最小的是 trigger_board ；
- PCB 打样的厚度一定要选择 1.2mm；
- 建议先掰开三块 PCB，用锉刀/砂纸打磨边缘后再焊接；若先焊后掰，注意不要损伤元件和走线；
- 掰板时戴护目镜，避免碎屑飞溅；

## 2. PCB与物料清单

- Gerber文件1：[hardware/gerber/Gerber_PCB1.zip](../hardware/gerber/Gerber_PCB1.zip)
- Gerber文件2：[hardware/gerber/Gerber_PCB2.zip](../hardware/gerber/Gerber_PCB2.zip)
- 原理图见：[hardware/schematic/SCH_Schematic.pdf](../hardware/schematic/SCH_Schematic.pdf)
- 完整 BOM 见：[hardware/bom/BOM_v0.1.csv](../hardware/bom/BOM_v0.1.csv)（可在线预览）[hardware/bom/BOM_v0.1.xlsx](../hardware/bom/BOM_v0.1.xlsx)（可编辑源文件）

建议移步至 Oshwhub 进行 PCB 打样：
- 链接：https://oshwhub.com/big_smoke/project_bniojffv

## 3. 工具和耗材

| 工具 | 是否必需 | 说明 |
|---|---|---|
| 烙铁 | 必需 | 建议 300-350°C |
| 焊锡 | 必需 | 0.6-0.8mm 含铅或无铅 |
| 锡膏 | 可选 | 中温含铅或无铅锡膏 |
| 万用表 | 必需 | 测通断、电压 |
| 螺丝刀 | 必需 | 拧螺丝 |
| 强力胶 | 必需 | 固定零件 |
| 加热台 | 可选 | 贴片焊接 |


## 4. 模型及3D打印参数

- stl 文件：[enclosure/stl](../enclosure/stl)
- step 文件：[enclosure/step](../enclosure/step)
- 3mf 文件：[enclosure/3mf](../enclosure/3mf)

建议移步至 MakerWorld 进行 3D 打印：
- MakerWorld 国际站：https://makerworld.com/zh/models/3447109-handheld-thermal-imager-casing#profileId-3931800
- MakerWorld 中国站：https://makerworld.com.cn/zh/models/3109405-shou-chi-re-cheng-xiang-yi-wai-ke#profileId-3661945

## 5. 装配步骤

### 5.1 建议焊接步骤

1. 焊接 main_board PCB 上的 TP5400 电源芯片及其外围电路，焊接 Type-C 母座及其外围电路；
2. 将 USB 5V 接入 Type-C ，用万用表测 TP5400 输出端电压是否位于 3.9V ~ 5V 之间；
3. 将单节锂电池临时接入 H2 电池接口，正极接 VBAT ，负极接 GND （注意正负极，不要接反），用万用表测 TP5400 电压输出端电压是否为 5V ；
4. 若步骤 2 和步骤 3 的测试结果均正常，则焊接 SGM2205 芯片及其外围电路，焊好后用万用表测 SGM2205 电压输出端电压是否为 3.3V ；
5. 若前 4 个步骤的测试结果均正常，则继续焊接其他元件；
6. 焊接 sensor_board PCB 以及 trigger_board PCB 上的元件，TOF400C 模块暂时不焊（如果用户选择安装）；

### 5.2 外壳处理步骤

1. 3D打印所有零件；
2. 拆除 main_body 零件的悬垂支撑，并用锉刀和砂纸对支撑面进行处理，使其尽可能平滑；
3. 若打样了 PCB1，则组合 top_1 零件和 top_2 零件，若打样了 PCB2，则组合 top_1 零件和 top_2_no_tof 零件，并用胶水进行固定；

### 5.3 总装配步骤

1.  在 trigger_board PCB 的焊点上分别焊 2 根导线（导线长度 > 50mm）；
2.  使用 M2 螺丝将 trigger_board PCB 固定在 main_body 零件上的指定位置；
3.  组合 trigger 零件和 main_body 零件；
4.  将 18650 电池放入电池盒中，再将电池盒放入 main_body 零件握柄的预留位置，并将电池电源线穿过 main_body 零件的内部；
5.  将 bottom 零件与 main_body 零件进行组合；
6.  将电池电源线焊接到 main_board PCB 上的 H2 电池接口（注意正负极，不要焊反）；
7.  将 trigger_board PCB 的两根导线焊接到 main_board PCB 上 H1 接口；
8.  将 ZH1.5 7P 排线的其中一端插入 main_board PCB 上的母座；
9.  将屏幕排线穿过 main_board PCB 上的缺口，并插入 main_board PCB 的 FPC 连接器中；
10. 使用 M2 螺丝将 main_board PCB 固定在 main_body 零件上；
11. 将 6mm 激光管放入组合完毕的 top 零件上的预留位置；
12. 如果打样了 PCB1，则将 TOF400C 模块放入 top 零件上的预留位置，若打样了 PCB2，则忽略本步骤；
13. 使用 M2 螺丝将 sensor_board PCB 固定在 top 零件上，若使用 PCB1：焊接 TOF400C 模块排针和激光管导线，若使用 PCB2：只焊接激光管导线，忽略 TOF400C；
14. 将 ZH1.5 7P 排线的另一端插入 sensor_board PCB 上的接口，随后将 top 零件与 main_body 零件进行组合；
15. 将屏幕放入 front 零件的限位槽中，并将 front 零件与 main_body 零件进行组合；
16. 将四个按键帽固定在开关按键上，带电源标识的按键帽在左侧，带实心圆标志的按键帽在右侧，箭头朝上的按键帽在上侧，箭头朝下的按键帽在下侧；
17. 如果按键帽松动，可以视情况使用胶水进行固定（不要让胶水流进开关按键的内部）；

## 6. 固件烧录

1. 安装 CH340 USB 转串口驱动；
2. 下载并打开 flash_download_tool (https://dl.espressif.com/public/flash_download_tool.zip)；
3. 选择 ESP32-S3 、Develop 、UART 、OK；
4. 将装置通过 USB 数据线连接到电脑，并按下装置的电源开关；
5. 从[firmware/bin](../firmware/bin)中选择要烧录的固件，若安装 TOF400C 模块且使用 IPS 屏幕，选择 firmware_tof_ips.bin；若不安装 TOF400C 模块且使用 IPS 屏幕，选择 firmware_no_tof_ips.bin；若安装 TOF400C 模块且使用 TFT 屏幕，选择 firmware_tof_tft.bin；若不安装 TOF400C 模块且使用 tft 屏幕，选择 firmware_no_tof_tft.bin；
6. 设置起始地址为 0x0 ，选择 COM 口，波特率设为 115200，如图[docs/images/firmware_tools.png](../docs/images/firmware_tools.png)所示；
 - 本项目 Release 中提供的是合并固件，起始地址填 0x0。
 - 若自行编译，请按 ESP-IDF 分区地址分别烧录：bootloader 0x0、partition-table 0x8000、app 0x10000。
7. 点击 START 等待烧录完成；

## 7. 上电测试

1. 按下开关，等待大约 1s；
2. 将传感器对准热源；
3. 观察屏幕是否显示热像图；

## 8. 许可证

CC BY-NC-SA 4.0 ，见 [../LICENSE.md](../LICENSE.md)