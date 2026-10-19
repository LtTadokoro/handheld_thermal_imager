/**
 * @copyright (C) 2017 Melexis N.V.
 * @license Apache 2.0
 * 
 * Modified for ESP-IDF platform - all I2C implementations are now embedded.
 */

#ifndef _MLX90640_API_H_
#define _MLX90640_API_H_

#include <stdint.h>
#include <stdbool.h>


// ---------------------------------------------------------------------
// 缓冲区大小
// ---------------------------------------------------------------------
#define EEDATA_SIZE     832
#define TEMP_SIZE       768
#define FRAME_SIZE      834
#define MATRIX_SIZE     768


// ---------------------------------------------------------------------
// 设置宏定义
// ---------------------------------------------------------------------
#define Resolution_16Bit    0x00
#define Resolution_17Bit    0x01
#define Resolution_18Bit    0x10
#define Resolution_19Bit    0x11
#define RefreshRate_halfHz  0x00
#define RefreshRate_1Hz     0x01
#define RefreshRate_2Hz     0x02
#define RefreshRate_4Hz     0x03
#define RefreshRate_8Hz     0x04
#define RefreshRate_16Hz    0x05
#define RefreshRate_32Hz    0x06
#define RefreshRate_64Hz    0x07


#define RefreshRate_2Hz_idx     1
#define RefreshRate_4Hz_idx     2
#define RefreshRate_8Hz_idx     3
#define RefreshRate_16Hz_idx    4
#define RefreshRate_32Hz_idx    5


// ---------------------------------------------------------------------
// 官方 API 返回的错误代码
// ---------------------------------------------------------------------
#define MLX90640_NO_ERROR                   0
#define MLX90640_I2C_NACK_ERROR             1
#define MLX90640_I2C_WRITE_ERROR            2
#define MLX90640_BROKEN_PIXELS_NUM_ERROR    3
#define MLX90640_OUTLIER_PIXELS_NUM_ERROR   4
#define MLX90640_BAD_PIXELS_NUM_ERROR       5
#define MLX90640_ADJACENT_BAD_PIXELS_ERROR  6
#define MLX90640_EEPROM_DATA_ERROR          7
#define MLX90640_FRAME_DATA_ERROR           8
#define MLX90640_MEAS_TRIGGER_ERROR         9

// ---------------------------------------------------------------------
// 寄存器地址及掩码 (与官方保持一致)
// ---------------------------------------------------------------------
#define MLX90640_EEPROM_START_ADDRESS       0x2400
#define MLX90640_EEPROM_DUMP_NUM            832
#define MLX90640_PIXEL_DATA_START_ADDRESS   0x0400
#define MLX90640_PIXEL_NUM                  768
#define MLX90640_LINE_NUM                   24
#define MLX90640_COLUMN_NUM                 32
#define MLX90640_LINE_SIZE                  32
#define MLX90640_AUX_DATA_START_ADDRESS     0x0700
#define MLX90640_AUX_NUM                    64
#define MLX90640_STATUS_REG                 0x8000
#define MLX90640_INIT_STATUS_VALUE          0x0030
#define MLX90640_CTRL_REG                   0x800D

#define POW2(x)                             pow(2, (double)(x))
#define SCALEALPHA                          0.000001

extern uint8_t refreshrate_idx;


// ---------------------------------------------------------------------
// 数据结构 (官方定义，未修改)
// ---------------------------------------------------------------------
typedef struct {
    int16_t kVdd;
    int16_t vdd25;
    float KvPTAT;
    float KtPTAT;
    uint16_t vPTAT25;
    float alphaPTAT;
    int16_t gainEE;
    float tgc;
    float cpKv;
    float cpKta;
    uint8_t resolutionEE;
    uint8_t calibrationModeEE;
    float KsTa;
    float ksTo[5];
    int16_t ct[5];
    uint16_t alpha[768];
    uint8_t alphaScale;
    int16_t offset[768];
    int8_t kta[768];
    uint8_t ktaScale;
    int8_t kv[768];
    uint8_t kvScale;
    float cpAlpha[2];
    int16_t cpOffset[2];
    float ilChessC[3];
    uint16_t brokenPixels[5];
    uint16_t outlierPixels[5];
} paramsMLX90640;

extern paramsMLX90640 params;

// ---------------------------------------------------------------------
// 官方 API 函数声明 (保持不变)
// ---------------------------------------------------------------------
int MLX90640_DumpEE(uint8_t slaveAddr, uint16_t *eeData);
int MLX90640_GetFrameData(uint8_t slaveAddr, uint16_t *frameData);
int MLX90640_ExtractParameters(uint16_t *eeData, paramsMLX90640 *mlx90640);
float MLX90640_GetVdd(uint16_t *frameData, const paramsMLX90640 *params);
float MLX90640_GetTa(uint16_t *frameData, const paramsMLX90640 *params);
void MLX90640_GetImage(uint16_t *frameData, const paramsMLX90640 *params, float *result);
void MLX90640_CalculateTo(uint16_t *frameData, const paramsMLX90640 *params, float emissivity, float tr, float *result);
int MLX90640_SetResolution(uint8_t slaveAddr, uint8_t resolution);
int MLX90640_GetCurResolution(uint8_t slaveAddr);
int MLX90640_SetRefreshRate(uint8_t slaveAddr, uint8_t refreshRate);
int MLX90640_GetRefreshRate(uint8_t slaveAddr);
int MLX90640_GetSubPageNumber(uint16_t *frameData);
int MLX90640_GetCurMode(uint8_t slaveAddr);
int MLX90640_SetInterleavedMode(uint8_t slaveAddr);
int MLX90640_SetChessMode(uint8_t slaveAddr);
void MLX90640_BadPixelsCorrection(uint16_t *pixels, float *to, int mode, paramsMLX90640 *params);

float MLX90640_CalculateSinglePixel(uint16_t *frameData, const paramsMLX90640 *params, int pixelIndex, float emissivity, float tr);

void MLX90640_Init(void);

void Set_RefreshRate_2Hz(void);
void Set_RefreshRate_4Hz(void);
void Set_RefreshRate_8Hz(void);
void Set_RefreshRate_16Hz(void);
void Set_RefreshRate_32Hz(void);

#endif