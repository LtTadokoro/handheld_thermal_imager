#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>
#include <esp_log.h>
#include "image_processing.h"
#include "buffer.h"
#include "display.h"
#include "ST7789_driver.h"
#include "task_management.h"

static const char *TAG = "IMAGE";

uint8_t scheme_idx = SCHEME_PSEUDO1_INDEX;
uint8_t interpolation_method = ON;  // 1为双线性，0为最邻近
uint8_t cold_crossing = ON;  // 冷点绘制，默认开启
uint8_t hot_crossing = ON;   // 热点绘制，默认开启
uint8_t temp_units_type = CELSIUS;	// 默认摄氏度

float hot_temp = 0, cold_temp = 0;
int hot_x, hot_y, cold_x, cold_y;

float vdd, tr, ta; // 反射温度，tr = Ta - 8.0

/**
 * @brief 将矩阵转置
 * @param src  输入矩阵（按行存储，rows 行，cols 列）
 * @param dst  输出矩阵（按行存储，cols 行，rows 列），大小必须与 src 相同
 * @param rows 输入矩阵行数（本例为 24）
 * @param cols 输入矩阵列数（本例为 32）
 */
void transpose(const float *src, float *dst, int rows, int cols) 
{
    int r, c;
    for (r = 0; r < rows; r++) 
	{
        for (c = 0; c < cols; c++) 
		{
            // 原 (r, c) → 转置后 (c, r)
            // 目标一维索引：c * rows + r
            dst[c * rows + r] = src[r * cols + c];
        }
    }
}

void get_hot_cold_points(float *image_data, uint16_t subPage, uint8_t mode, const paramsMLX90640 *params, float emissivity, float tr, float *hot_temp, float *cold_temp, int *hot_x, int *hot_y, int *cold_x, int *cold_y) 
{
    // 获取当前子页和模式
    //uint16_t subPage = frameData[833];
    //uint8_t mode = (frameData[832] & 0x1000) >> 5;
    
    int max_idx = -1, min_idx = -1;
    float max_val = -1e38f, min_val = 1e38f;
    
    for (int i = 0; i < 768; i++) 
	{
        // 计算该像素的 pattern（与 GetImage/CalculateTo 中的逻辑一致）
		
        int ilPattern = i / 32 - (i / 64) * 2;
        int chessPattern = ilPattern ^ (i - (i / 2) * 2);
        int pattern = (mode == 0) ? ilPattern : chessPattern;
        // 跳过不属于当前子页的像素
        if (pattern != subPage) continue;
		
        float v = image_data[i];
        if (v > max_val) 
		{ 
			max_val = v; max_idx = i; 
		}
        if (v < min_val) 
		{ 
			min_val = v; min_idx = i; 
		}
    }
    
    // 防御：如果没有找到有效像素（理论上不会）
    if (max_idx == -1 || min_idx == -1) 
	{
        *cold_temp = 0; *cold_temp = 0;
        return;
    }
    
    *hot_temp = max_val;
    *cold_temp = min_val;
    
    // 计算屏幕坐标（与原代码相同）
    int src_x_max = max_idx % SRC_WIDTH;
    int src_y_max = max_idx / SRC_WIDTH;
    int src_x_min = min_idx % SRC_WIDTH;
    int src_y_min = min_idx / SRC_WIDTH;
    float scale_x = (float)(IMAGE_WIDTH - 1) / (SRC_WIDTH - 1);
    float scale_y = (float)(IMAGE_HEIGHT - 1) / (SRC_HEIGHT - 1);
    int dst_x_max = (int)(src_x_max * scale_x + 0.5f);
    int dst_y_max = (int)(src_y_max * scale_y + 0.5f);
    int dst_x_min = (int)(src_x_min * scale_x + 0.5f);
    int dst_y_min = (int)(src_y_min * scale_y + 0.5f);
    
    #ifdef EDGE_WIDTH
        dst_x_max += EDGE_WIDTH;
        dst_y_max += EDGE_WIDTH;
        dst_x_min += EDGE_WIDTH;
        dst_y_min += EDGE_WIDTH;
    #endif
    
    *hot_x = dst_x_max;
    *hot_y = dst_y_max;
    *cold_x = dst_x_min;
    *cold_y = dst_y_min;
}

// 最临近插值
void Neighbor_Interpolation(float *originalData, float *processedData)
{
	uint16_t dst_w = IMAGE_WIDTH;
    uint16_t dst_h = IMAGE_HEIGHT;
    uint8_t src_w = SRC_WIDTH;
    uint8_t src_h = SRC_HEIGHT;

    for (uint16_t y = 0; y < dst_h; y++)
    {
        // 计算目标行对应的原始行索引（整数除法，无浮点）
        uint8_t src_y = (uint16_t)(y * src_h) / dst_h;
        uint16_t src_row = (uint16_t)src_y * src_w;   // 原始行起始索引

        for (uint16_t x = 0; x < dst_w; x++)
        {
            uint8_t src_x = (uint16_t)(x * src_w) / dst_w;
            // 索引计算：必须用 32 位，防止 y*dst_w 溢出（最大 239*320=76480）
            uint32_t dst_idx = (uint32_t)y * dst_w + x;
            processedData[dst_idx] = originalData[src_row + src_x];
        }
    }
}

// 双线性插值
void Double_LinearInterpolation(float *originalData, float *processedData)
{
	//插值后的图像像素尺寸
    uint16_t dst_w = IMAGE_WIDTH;
    uint16_t dst_h = IMAGE_HEIGHT;
	//插值前的像素尺寸
    uint8_t  src_w = SRC_WIDTH;
    uint8_t  src_h = SRC_HEIGHT;

    // 预计算步长（浮点，避免内循环重复除法）
    float x_scale = (float)(src_w - 1) / (dst_w - 1);
    float y_scale = (float)(src_h - 1) / (dst_h - 1);

    for (uint16_t y = 0; y < dst_h; y++)
    {
        float src_y_f = y * y_scale;
        uint8_t y1 = (uint8_t)src_y_f;          // 0~23，直接截断
        uint8_t y2 = (y1 < src_h - 1) ? (y1 + 1) : y1;
        float fy = src_y_f - y1;                 // 垂直方向权重
        float fy1 = 1.0f - fy;

        // 预计算原始图像中的行偏移（uint16_t足够，最大23*32=736）
        uint16_t row1 = (uint16_t)y1 * src_w;
        uint16_t row2 = (uint16_t)y2 * src_w;

        for (uint16_t x = 0; x < dst_w; x++)
        {
            float src_x_f = x * x_scale;
            uint8_t x1 = (uint8_t)src_x_f;       // 0~31
            uint8_t x2 = (x1 < src_w - 1) ? (x1 + 1) : x1;
            float fx = src_x_f - x1;              // 水平方向权重
            float fx1 = 1.0f - fx;

            // 取四个相邻像素值
            float v00 = originalData[row1 + x1];
            float v10 = originalData[row1 + x2];
            float v01 = originalData[row2 + x1];
            float v11 = originalData[row2 + x2];

            // 双线性插值
            float v0 = v00 * fx1 + v10 * fx;
            float v1 = v01 * fx1 + v11 * fx;
            // 目标索引：必须用32位，因为最大239*320+319=76799 > 65535
            uint32_t dst_idx = (uint32_t)y * dst_w + x;
            processedData[dst_idx] = v0 * fy1 + v1 * fy;
        }
    }
}

// 插值函数
void Interpolation(uint8_t method, float *originalData, float *processedData)
{
	if(method)
	{
		Double_LinearInterpolation(originalData, processedData);
	}
	else
	{
		Neighbor_Interpolation(originalData, processedData);
	}
}

// 数值映射（0~255）
void Value_Mapping(float *originalImage, uint8_t *mappedImage, float maxVal, float minVal)
{
    const int length = IMAGE_WIDTH * IMAGE_HEIGHT;
    if (length <= 0) return;

    // ---------- 第一步：同时找出最小值和最大值 ----------
    // 处理所有值相等的情况（避免除零）
    if (maxVal == minVal) {
        // 统一映射到中间灰度
        memset(mappedImage, 127, length);
    }
    // 计算缩放系数
    const float scale = 255.0f / (maxVal - minVal);

    // ---------- 第二步：映射并存储 ----------
	for (int i = 0; i < length; i++) {
        // 线性映射到 [0, 255]
        float val = (originalImage[i] - minVal) * scale;
		// 伽马变换
		//float gamma = 1.5;  // <1 提亮暗部，>1 压暗亮部
		//float normalized = val / 255.0f;
		//normalized = powf(normalized, gamma);
        // 四舍五入（加0.5后取整）
        int intVal = (int)(val + 0.5f);
        // 防止浮点误差导致越界
        if (intVal < 0) intVal = 0;
        if (intVal > 255) intVal = 255;
        mappedImage[i] = (uint8_t)intVal;
    }
}

/** 绘制矩形（自动切除越界部分）
 * @param x1 矩形左下角横坐标
 * @param y1 矩形左下角纵坐标
 * @param x2 矩形右上角横坐标
 * @param y2 矩形右上角纵坐标
 * @param *buffer 要写入的缓冲区
 * @param color 矩形颜色
 * @return 无
*/
void draw_rect_on_image(uint16_t x1, uint16_t x2, uint16_t y1, uint16_t y2, uint16_t *buffer, uint16_t color)
{
	uint16_t draw_x1, draw_x2, draw_y1, draw_y2;
	if(x1 > EDGE_WIDTH)
	{
		draw_x1 = x1;
	}
	else
	{
		draw_x1 = EDGE_WIDTH;
	}
	if(x2 < IMAGE_WIDTH + EDGE_WIDTH - 1)
	{
		draw_x2 = x2;
	}
	else
	{
		draw_x2 = IMAGE_WIDTH + EDGE_WIDTH - 1;
	}
	if(y1 > EDGE_WIDTH)
	{
		draw_y1 = y1;
	}
	else
	{
		draw_y1 = EDGE_WIDTH;
	}
	if(y2 < IMAGE_HEIGHT + EDGE_WIDTH - 1)
	{
		draw_y2 = y2;
	}
	else
	{
		draw_y2 = IMAGE_HEIGHT + EDGE_WIDTH - 1;
	}
	if (draw_x1 > draw_x2 || draw_y1 > draw_y2) 
	{
		return;
	}
	for(uint16_t y = draw_y1; y <= draw_y2; y++)
	{
		uint32_t row_start = y * LCD_WIDTH + draw_x1;
		for (uint16_t x = draw_x1; x <= draw_x2; x++) 
		{
            buffer[row_start + (x - draw_x1)] = color;
        }
	}
}

/** 绘制十字
 * @param crossing_x 十字中心横索引坐标
 * @param crossing_y 十字中心纵索引坐标
 * @param crossing_length 十字悬臂单臂长
 * @param crossing_thick 十字壁厚
 * @param *buffer 要写入的缓冲区
 * @param color 十字颜色 
 * @return 无
*/
void draw_crossing(uint16_t crossing_x, uint16_t crossing_y, uint16_t crossing_length, uint16_t crossing_thick, uint16_t *buffer, uint16_t color)
{
	uint16_t half_thick = (crossing_thick - 1) / 2;
	//先画水平悬臂
	draw_rect_on_image(crossing_x - crossing_length, crossing_x + crossing_length, crossing_y - half_thick, crossing_y + half_thick, buffer, color);
	//在画垂直悬臂
	draw_rect_on_image(crossing_x - half_thick, crossing_x + half_thick, crossing_y - crossing_length, crossing_y + crossing_length, buffer, color);
}
