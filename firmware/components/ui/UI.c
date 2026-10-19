#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "UI.h"
#include "ugui.h"
#include "display.h"
#include "ST7789_driver.h"
#include "SD_card.h"


// 显示区域在屏幕最下方
#define AREA_XS       0
#define AREA_YS       (LCD_HEIGHT - 25 - 1)
#define AREA_XE       (LCD_WIDTH - 1)
#define AREA_YE       (LCD_HEIGHT - 1)

// 字体高度
#define FONT_HEIGHT   14
// 显示区域可用高度为25，居中偏移 = (25 - 12) / 2 = 6（整数取6）
#define CENTER_OFFSET 7

// 居中时的文字Y坐标（文字左上角）
#define TEXT_Y_CENTER (AREA_YS + CENTER_OFFSET)  // 295+6=301
#define TITEL_Y_CENTER 0

// 电池图标左上角坐标
#define BATTERY_ICON_X       220
#define BATTERY_ICON_Y       4
#define BATTERY_WIDTH        14 //不计凸起
#define BATTERY_HEIGHT       7
#define BATTERY_FRAME_COLOR  0xE0FF
#define BATTERY_LOW_COLOR    0x00F8
#define BATTERY_HALF_COLOR   0xE0FF
#define BATTERY_MEDIUM_COLOR 0xE007
#define BATTERY_FULL_COLOR   0xE007

// 温度数字
#define UPPER_TEMPREATURE_AREA_X   0
#define UPPER_TEMPREATURE_AREA_Y   0
#define LOWER_TEMPREATURE_AREA_X   0
#define LOWER_TEMPREATURE_AREA_Y   300
#define TEMPREATURE_AREA_WIDTH     60
#define TEMPREATURE_AREA_HEIGHT    15 -1
#define TEMPREATURE_UPPER_COLOR    0x61F8
#define TEMPREATURE_LOWER_COLOR    0x9F10

// 屏幕上方显示区域
#define TITLE_AREA_X         70
#define TITLE_AREA_Y         0
#define TITLE_AREA_WIDTH     139
#define TITLE_AREA_HEIGHT    14

// 颜色
#define SUCCESSED            0xE007
#define WARNING              0xE0FF
#define ERROR                0x00F8


#define SCROLL_STEPS         20   // 总步数（帧数）
#define STEP_PIXEL           2    // 每步移动像素数

UG_GUI my_gui;

typedef enum {
    ANIM_NONE,
    ANIM_SCROLL_DOWN,
    ANIM_SCROLL_UP
} AnimState;

typedef struct {
    char* current_text;
    char* next_text;
    AnimState state;
    int step_count;          // 当前已走步数 (0~SCROLL_STEPS)
    int current_offset;      // 当前文字的偏移（相对 TEXT_Y_CENTER）
    int next_offset;         // 新文字的偏移
    int current_target;      // 当前文字的目标偏移
    int next_target;         // 新文字的目标偏移
} MenuScroll;

MenuScroll scroll = {0};

int GetTextWidth(char* str) 
{
    int w = 0;
    const UG_FONT* font = &my_gui.font; // 或您保存的字体指针
    for (char* p = str; *p; p++) 
    {
        if (*p == '\n') break; // 仅单行
        w += font->widths ? font->widths[*p - font->start_char] : font->char_width;
    }
    return w;
}

void MenuScrollTo(char* new_text, int direction) 
{
    // direction: 1 向下滚动（新文字从上方进入），-1 向上滚动（新文字从下方进入）
    if (scroll.state != ANIM_NONE) return;  // 动画未完成

    // 保存当前文字（如果无当前文字，则直接显示新文字）
    if (scroll.current_text == NULL) 
    {
        scroll.current_text = new_text;
        scroll.state = ANIM_NONE;
        return;
    }

    scroll.next_text = new_text;
    scroll.step_count = 0;

    if (direction == 0) 
    {
        // 直接显示，不滚动
        scroll.current_text = new_text;
        scroll.next_text = NULL;
        scroll.state = ANIM_NONE;
        scroll.current_offset = 0;
        return;
    }
    else if (direction == -1) 
    {
        // 向下滚动：当前文字向下移出（偏移 +20），新文字从上方进入（偏移 -20 → 0）
        scroll.state = ANIM_SCROLL_DOWN;
        scroll.current_offset = 0;
        scroll.current_target = SCROLL_STEPS * STEP_PIXEL;   // +20
        scroll.next_offset = -SCROLL_STEPS * STEP_PIXEL;     // -20
        scroll.next_target = 0;
    } 
    else 
    { // direction == -1
        // 向上滚动：当前文字向上移出（偏移 -20），新文字从下方进入（偏移 +20 → 0）
        scroll.state = ANIM_SCROLL_UP;
        scroll.current_offset = 0;
        scroll.current_target = -SCROLL_STEPS * STEP_PIXEL;  // -20
        scroll.next_offset = SCROLL_STEPS * STEP_PIXEL;      // +20
        scroll.next_target = 0;
    }
}

void MenuScrollUpdate(void) 
{
    if (scroll.state == ANIM_NONE) return;

    // 检查是否完成
    if (scroll.step_count >= SCROLL_STEPS) 
    {
        // 动画结束
        scroll.current_text = scroll.next_text;
        scroll.next_text = NULL;
        scroll.state = ANIM_NONE;
        scroll.current_offset = 0;
        scroll.next_offset = 0;
        return;
    }

    // 移动一步
    if (scroll.current_offset < scroll.current_target) scroll.current_offset += STEP_PIXEL;
    else if (scroll.current_offset > scroll.current_target) scroll.current_offset -= STEP_PIXEL;

    if (scroll.next_offset < scroll.next_target) scroll.next_offset += STEP_PIXEL;
    else if (scroll.next_offset > scroll.next_target) scroll.next_offset -= STEP_PIXEL;

    scroll.step_count++;
}

void MenuScrollDraw(void) 
{
    UG_SetForecolor(C_WHEAT); // 白色字体
    // 清空显示区域
    UG_FillFrame(50, AREA_YS, AREA_XE, AREA_YE, C_BLACK);

    if (scroll.state != ANIM_NONE) 
    {
        // 绘制当前文字（可能正在移出）
        if (scroll.current_text) 
        {
            int y = TEXT_Y_CENTER + scroll.current_offset;
            int w = GetTextWidth(scroll.current_text);
            int x = (AREA_XE - AREA_XS + 1 - w) / 2 + AREA_XS;
            UG_PutString(88, y, scroll.current_text);
        }
        // 绘制新文字（可能正在移入）
        if (scroll.next_text) 
        {
            int y = TEXT_Y_CENTER + scroll.next_offset;
            int w = GetTextWidth(scroll.next_text);
            int x = (AREA_XE - AREA_XS + 1 - w) / 2 + AREA_XS;
            UG_PutString(88, y, scroll.next_text);
        }
    } 
    else 
    {
        // 稳定显示当前文字
        if (scroll.current_text) 
        {
            int w = GetTextWidth(scroll.current_text);
            int x = (AREA_XE - AREA_XS + 1 - w) / 2 + AREA_XS;
            UG_PutString(88, TEXT_Y_CENTER, scroll.current_text);
        }
    }
}

// 绘制电池图标外框
void Draw_battery_frame(void)
{
    UG_DrawFrame(BATTERY_ICON_X, BATTERY_ICON_Y, BATTERY_ICON_X + BATTERY_WIDTH, BATTERY_ICON_Y + BATTERY_HEIGHT, BATTERY_FRAME_COLOR); // 外框
    UG_DrawFrame(BATTERY_ICON_X + BATTERY_WIDTH + 1, BATTERY_ICON_Y + 2, BATTERY_ICON_X + BATTERY_WIDTH + 2, BATTERY_ICON_Y + BATTERY_HEIGHT - 2, BATTERY_FRAME_COLOR);  // 凸起
}

// 根据电量填充电池图标
void Fill_battery_frame(uint8_t voltage_level)
{
    UG_FillFrame(BATTERY_ICON_X + 1, BATTERY_ICON_Y + 1, BATTERY_ICON_X + BATTERY_WIDTH - 1, BATTERY_ICON_Y + BATTERY_HEIGHT - 1, C_BLACK); // 先清除
    switch (voltage_level)
    {
        case 1:
            UG_FillFrame(BATTERY_ICON_X + 2, BATTERY_ICON_Y + 2, BATTERY_ICON_X + 5, BATTERY_ICON_Y + BATTERY_HEIGHT - 2, BATTERY_LOW_COLOR);
            break;
        case 2:
            UG_FillFrame(BATTERY_ICON_X + 2, BATTERY_ICON_Y + 2, BATTERY_ICON_X + 7, BATTERY_ICON_Y + BATTERY_HEIGHT - 2, BATTERY_HALF_COLOR);
            break;
        case 3:
            UG_FillFrame(BATTERY_ICON_X + 2, BATTERY_ICON_Y + 2, BATTERY_ICON_X + 10, BATTERY_ICON_Y + BATTERY_HEIGHT - 2, BATTERY_MEDIUM_COLOR);
            break;
        case 4:
            UG_FillFrame(BATTERY_ICON_X + 2, BATTERY_ICON_Y + 2, BATTERY_ICON_X + 12, BATTERY_ICON_Y + BATTERY_HEIGHT - 2, BATTERY_FULL_COLOR);
            break;
    }
}

void DrawTitle(int16_t value)
{
    char text[25];
    UG_FillFrame(TITLE_AREA_X, TITLE_AREA_Y, BATTERY_ICON_X - 1, TITLE_AREA_HEIGHT, C_BLACK);

    if(value >= 0)
    {
        snprintf(text, sizeof(text), "%s%.1f%s", "range: ", value / 10.0, "cm");
        UG_SetForecolor(C_WHEAT);
    }
    else if(value == RANGE_INVALID_FLAG)
    {
        snprintf(text, sizeof(text), "%s%s", "range: ", "invalid");
        UG_SetForecolor(WARNING);
    }
    else if(value == RANGE_ERROR_FLAG)
    {
        snprintf(text, sizeof(text), "%s%s", "range: ", "error");
        UG_SetForecolor(ERROR);
    }
    else if(value == CAPTURE_SUCCESSFUL_FLAG)
    {
        snprintf(text, sizeof(text), "captured: %03u", (unsigned)image_idx);
        UG_SetForecolor(SUCCESSED);
    }
    else if(value == NO_SD_CARD_FLAG)
    {
        strcpy(text, "no SD card");
        UG_SetForecolor(ERROR);
    }
    UG_PutString(TITLE_AREA_X, TITEL_Y_CENTER, text);
    vTaskDelay(pdMS_TO_TICKS(1500));
    UG_FillFrame(TITLE_AREA_X, TITLE_AREA_Y, BATTERY_ICON_X - 1, TITLE_AREA_HEIGHT, C_BLACK);
}

void DrawTemperature(float upper_temp, float lower_temp, uint8_t temp_type)
{
    // 先清空两个区域
    UG_FillFrame(UPPER_TEMPREATURE_AREA_X, UPPER_TEMPREATURE_AREA_Y, TEMPREATURE_AREA_WIDTH, TEMPREATURE_AREA_HEIGHT, C_BLACK);
    UG_FillFrame(UPPER_TEMPREATURE_AREA_X, LOWER_TEMPREATURE_AREA_Y, TEMPREATURE_AREA_WIDTH, LOWER_TEMPREATURE_AREA_Y + TEMPREATURE_AREA_HEIGHT, C_BLACK);

    if(temp_type == 1)
    {
        upper_temp = upper_temp * 9 / 5 + 32;
        lower_temp = lower_temp * 9 / 5 + 32;
    }
    else if(temp_type ==2)
    {
        upper_temp = upper_temp + 273.15;
        lower_temp = lower_temp + 273.15;
    }

    char temp_1[6], temp_2[6];
    snprintf(temp_1, sizeof(temp_1), "%.1f", upper_temp);
    snprintf(temp_2, sizeof(temp_2), "%.1f", lower_temp);
    UG_SetForecolor(TEMPREATURE_UPPER_COLOR);
    UG_PutString(UPPER_TEMPREATURE_AREA_X, UPPER_TEMPREATURE_AREA_Y, temp_1);

    if(temp_type == 0)
    {
        UG_PutString(45, UPPER_TEMPREATURE_AREA_Y, "\x7F""C");
    }
    else if(temp_type == 1)
    {
        UG_PutString(45, UPPER_TEMPREATURE_AREA_Y, "\x7F""F");
    }
    else if(temp_type == 2)
    {
        UG_PutString(45, UPPER_TEMPREATURE_AREA_Y, "K");
    }

    
    UG_SetForecolor(TEMPREATURE_LOWER_COLOR);
    UG_PutString(LOWER_TEMPREATURE_AREA_X, LOWER_TEMPREATURE_AREA_Y, temp_2);

    if(temp_type == 0)
    {
        UG_PutString(45, LOWER_TEMPREATURE_AREA_Y, "\x7F""C");
    }
    else if(temp_type == 1)
    {
        UG_PutString(45, LOWER_TEMPREATURE_AREA_Y, "\x7F""F");
    }
    else if(temp_type == 2)
    {
        UG_PutString(45, LOWER_TEMPREATURE_AREA_Y, " K");
    }
}

void UI_Init(void)
{
    UG_Init(&my_gui, Write_Single_Pixel_Buffer, LCD_WIDTH, LCD_HEIGHT);
    Draw_battery_frame();
    UG_FontSelect(&FONT_8X14);
}