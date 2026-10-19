#ifndef _BUFFER_H_
#define _BUFFER_H_

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include "image_processing.h"


#define FRAME_BUF_COUNT 2

// 色彩方案枚举（与 GrayToPseColor 中的 case 对应）
typedef enum {
    COLOR_SCHEME_GRAY = 0,
    COLOR_SCHEME_METAL,
    COLOR_SCHEME_PSEUDO1,
    COLOR_SCHEME_PSEUDO2,
    COLOR_SCHEME_RAINBOW1,
    COLOR_SCHEME_RAINBOW2,
    COLOR_SCHEME_COUNT      // 方案总数
} color_scheme_t;

extern uint16_t *g_current_color_lut;

extern uint16_t *eeData;//832
extern uint16_t *FrameData[FRAME_BUF_COUNT];;// 834
extern float *OriginalDataMatrix;// 768
extern float *RotatedDataMatrix;// 768
extern float *InterpolationDataMatrix;// IMAGE_WIDTH*IMAGE_HEIGHT
extern uint8_t *GrayValueMatrix;// IMAGE_WIDTH*IMAGE_HEIGHT
extern uint16_t *RGBValueMatrix;// IMAGE_WIDTH*IMAGE_HEIGHT
extern uint16_t *DisplayBuffer;// LCD_WIDTH*LCD_HEIGHT

extern uint16_t *CurrentRGBValueMatrix;// LCD_WIDTH*LCD_HEIGHT

extern SemaphoreHandle_t raw_free_sem;
extern QueueHandle_t raw_ready_queue;
extern SemaphoreHandle_t display_mutex;
extern SemaphoreHandle_t image_mutex;


void alloc_psram_buffers(void);
void Pseudo_Color_Fast(uint8_t *grayImage, uint16_t *rgb565Image);
void Set_Color_Scheme(color_scheme_t scheme);
void Color_Scheme_Init(void);
void init_sync_objects(void);

#endif