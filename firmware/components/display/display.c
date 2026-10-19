#include <string.h>
#include <freeRTOS/FreeRTOS.h>
#include <freeRTOS/task.h>
#include "display.h"
#include "ST7789_driver.h"
#include "buffer.h"

const uint8_t xy_offset = ((LCD_WIDTH - IMAGE_WIDTH) / 2 );

// 将热像图映射并写入显示缓冲区的指定位置
void Write_Buffer(uint16_t *thermal_image, uint16_t *common_buffer)
{
	for (int y = 0; y < IMAGE_HEIGHT ; y++) 
	{
        uint32_t dst_row = ((EDGE_WIDTH + y) * LCD_WIDTH + EDGE_WIDTH);
        uint32_t src_row = (IMAGE_WIDTH * y);
        memcpy(&common_buffer[dst_row], &thermal_image[src_row], IMAGE_WIDTH * sizeof(uint16_t));
    }
}

//向公共缓冲区写入一个像素的函数接口，因为图形库的要求，缓冲区直接全局调用（for uGUI）（x:0-239,y:0-319）
void Write_Single_Pixel_Buffer(int16_t x, int16_t y, uint16_t color)
{
    //判断坐标是否越界
    if((x < 0) || (x > (LCD_WIDTH - 1)))
    {
        return;
    }
    if((y < 0) || (y > (LCD_HEIGHT - 1)))
    {
        return;
    }
    //判断位置是否与热像图重合，如重合则直接返回不写入
    if((y > (EDGE_WIDTH - 0)) && (y < (LCD_HEIGHT - (LCD_HEIGHT - IMAGE_HEIGHT - EDGE_WIDTH) - 0)))
    {
        return;
    }
    DisplayBuffer[y * LCD_WIDTH + x] = color;
}

void Fill_Black_Background(uint16_t *common_buffer) 
{
    uint32_t len = LCD_WIDTH * LCD_HEIGHT;
    memset(common_buffer, 0x0000, (2 * len));
    st7789_flush(0, LCD_WIDTH, 0, LCD_HEIGHT, common_buffer);
}

void Draw_Color_Strip(void)
{
    uint16_t width = STRIP_WIDTH;           // 色带宽度
    uint16_t height = IMAGE_HEIGHT;         // 与热像图等高
    if (width == 0 || height == 0) return;

    // 色带放在屏幕最左侧，x=0 到 width-1
    for (uint16_t y = 0; y < height; y++) 
    {
        // 灰度从上到下递减（高温在上，低温在下）
        uint8_t gray = (uint8_t)(255 - (y * 255) / (height - 1));
        for (uint16_t x = 0; x < width; x++) 
        {
            DisplayBuffer[(y + EDGE_WIDTH) * LCD_WIDTH + x] = g_current_color_lut[gray];
        }
    }
}