#ifndef _DISPLAY_H_
#define _DISPLAY_H_

#include <stdio.h>
#include "image_processing.h"
#include "buffer.h"

#define EDGE_WIDTH xy_offset
//侧面展示色彩带厚度（宽度<边缘宽度15）
#define STRIP_WIDTH     7

extern const uint8_t xy_offset;

void Write_Buffer(uint16_t *thermal_image, uint16_t *common_buffer);
void Fill_Black_Background(uint16_t *common_buffer);
void Draw_Color_Strip(void);
void Write_Single_Pixel_Buffer(int16_t x, int16_t y, uint16_t color);

void display(void);

#endif