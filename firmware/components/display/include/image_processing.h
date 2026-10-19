
#ifndef _IMAGE_PROCESSING_H_
#define _IMAGE_PROCESSING_H_

#include <stdio.h>
#include "MLX90640_API.h"


#define ON          1
#define OFF         0
#define CELSIUS     0
#define FAHRENHEIT  1
#define KELVIN      2

#define EMISSIVITY 0.95 // 反射率

//传感器像素尺寸
#define SRC_WIDTH           24
#define SRC_HEIGHT          32
//实际要显示的图片像素尺寸
#define IMAGE_WIDTH         210
#define IMAGE_HEIGHT        280
//十字标记点臂长（总长度2l+1）
#define CROSSING_LENGTH     7
//十字标记点壁厚（总厚度2t+1）
#define CROSSING_THICK      3
//十字标记颜色
#define HOT_POINT_COLOR     0xFFFF
#define COLD_POINT_COLOR    0x0000

#define SCHEME_GRAY_INDEX       10
#define SCHEME_METAL_INDEX      11
#define SCHEME_PSEUDO1_INDEX    12
#define SCHEME_PSEUDO2_INDEX    13
#define SCHEME_RAINBOW1_INDEX   14
#define SCHEME_RAINBOW2_INDEX   15

void transpose(const float *src, float *dst, int rows, int cols);
void get_hot_cold_points(float *image_data, uint16_t subPage, uint8_t mode, const paramsMLX90640 *params, float emissivity, float tr, float *hot_temp, float *cold_temp, int *hot_x, int *hot_y, int *cold_x, int *cold_y);
void Neighbor_Interpolation(float *originalData, float *processedData);
void Double_LinearInterpolation(float *originalData, float *processedData);
void Interpolation(uint8_t method, float *originalData, float *processedData);
void Value_Mapping(float *originalImage, uint8_t *mappedImage, float maxVal, float minVal);

void draw_crossing(uint16_t crossing_x, uint16_t crossing_y, uint16_t crossing_length, uint16_t crossing_thick, uint16_t *buffer, uint16_t color);

extern uint8_t interpolation_method;
extern uint8_t scheme_idx;
extern uint8_t cold_crossing;
extern uint8_t hot_crossing;
extern uint8_t temp_units_type;

extern float hot_temp, cold_temp;
extern int hot_x, hot_y, cold_x, cold_y;

extern float vdd, tr, ta;


#endif