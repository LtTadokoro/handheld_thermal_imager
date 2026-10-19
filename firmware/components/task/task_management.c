#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_timer.h>
#include "task_management.h"
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"
#include "vl53l1x.h"
#include "ST7789_driver.h"
#include "image_processing.h"
#include "buffer.h"
#include "display.h"
#include "gpio.h"
#include "SD_card.h"
#include "adc.h"
#include "UI.h"
#include "menu.h"
#include "nvs_wr.h"

#include "config.h"


#define KEY_DEBOUNCE_MS     30
#define KEY_LONG_PRESS_MS   1000
#define KEY_DOUBLE_CLICK_MS 500

#define TRIGGER_KEY_MASK    0x01
#define DOWN_KEY_MASK       0x02
#define UP_KEY_MASK         0x04
#define RIGHT_KEY_MASK      0x08

#define SINGLE_MASK         0x40
#define DOUBLE_MASK         0x20
#define LONG_MASK           0x10

uint8_t voltage_level = 4;

static const char *TAG = "TASK";

TaskHandle_t polling_task_handle = NULL;
TaskHandle_t button_task_handle = NULL;
TaskHandle_t image_shot_task_handle = NULL;
TaskHandle_t ranging_task_handle = NULL;

// 按键状态枚举
typedef enum {
    KEY_STATE_IDLE = 0,
    KEY_STATE_DEBOUNCE,    // 消抖等待中
    KEY_STATE_PRESSED,     // 已确认按下
    KEY_STATE_WAIT_DOUBLE, // 释放后等待双击超时
} key_state_t;

// 按键结构体（务必初始化为0）
typedef struct {
    gpio_num_t pin;
    key_state_t state;
    uint64_t last_tick_ms;       // 上次状态跳变的时间戳（毫秒）
    uint64_t press_tick_ms;      // 确认按下的时刻
    uint64_t last_click_tick_ms; // 上次单击的时刻
    bool is_long_triggered;      // 长按是否已触发
    bool is_event_handled;       // 关键标志：本次按键动作是否已产生回调
} key_handle_t;

static key_handle_t s_keys[4] = {0};

// 获取当前毫秒的64位安全函数
static inline uint64_t get_tick_ms(void)
{
    return esp_timer_get_time() / 1000; // 64位微秒转毫秒
}

//热像图任务
void read_task(void *param)
{
    uint8_t idx = 0;
    while(1)
    {
        xSemaphoreTake(raw_free_sem, portMAX_DELAY);
        int ret = MLX90640_GetFrameData(MLX90640_DEFAULT_ADDR, FrameData[idx]);
        if (ret >= 0)
        {
            // 发送队列
            xQueueSend(raw_ready_queue, &idx, portMAX_DELAY);
        }
        else
        {
            // 读取失败，立即归还缓冲
            xSemaphoreGive(raw_free_sem);
        }
        idx ^= 1;       // 切换到下一个缓冲（乒乓）
        
        vTaskDelay(1);
    }
}

// 转换任务
void transform_task(void *param)
{
    uint8_t idx;
    while (1) 
    {
        // 等待 read_task 发来的原始数据就绪索引
        xQueueReceive(raw_ready_queue, &idx, portMAX_DELAY);

        // 提取子页号和模式，后续不再需要 FrameData[idx]
        uint16_t subPage = FrameData[idx][833];
        uint8_t mode = (FrameData[idx][832] & 0x1000) >> 5;

        ta = MLX90640_GetTa(FrameData[idx], &params);   // 近似环境反射温度
        tr = ta - 8.0f;
        MLX90640_CalculateTo(FrameData[idx], &params, EMISSIVITY, tr, OriginalDataMatrix);

        // 在处理完原始帧后立即归还缓冲，让 read_task 可以继续读下一帧
        xSemaphoreGive(raw_free_sem);

        transpose(OriginalDataMatrix, RotatedDataMatrix, SRC_WIDTH, SRC_HEIGHT);    // 转置，适配竖屏
        get_hot_cold_points(RotatedDataMatrix, subPage, mode, &params, EMISSIVITY, tr, &hot_temp, &cold_temp, &hot_x, &hot_y, &cold_x, &cold_y);    // 获取最热点最冷点坐标
        Interpolation(interpolation_method, RotatedDataMatrix, InterpolationDataMatrix);    //插值
        Value_Mapping(InterpolationDataMatrix, GrayValueMatrix, hot_temp, cold_temp);      // 灰度映射

        xSemaphoreTake(image_mutex, portMAX_DELAY);                                 // 互斥锁，截图任务用
        Pseudo_Color_Fast(GrayValueMatrix, RGBValueMatrix);                        // 灰度 转 RGB565
        CurrentRGBValueMatrix = RGBValueMatrix;                                 // 供截图任务使用
        xSemaphoreGive(image_mutex);

        xSemaphoreTake(display_mutex, portMAX_DELAY);
        Write_Buffer(RGBValueMatrix, DisplayBuffer);                             // 将数据写入公共缓冲区
        if(cold_crossing)                                                       //绘制热点冷点
        {
            draw_crossing(cold_x, cold_y, CROSSING_LENGTH, CROSSING_THICK, DisplayBuffer, COLD_POINT_COLOR);
        }
        if(hot_crossing)
        {
            draw_crossing(hot_x, hot_y, CROSSING_LENGTH, CROSSING_THICK, DisplayBuffer, HOT_POINT_COLOR);
        }
        xSemaphoreGive(display_mutex);

        vTaskDelay(1);
    }
}

// 显示任务
void display_task(void *param)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(1)
    {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(30));
        xSemaphoreTake(display_mutex, portMAX_DELAY);
        st7789_flush(0, LCD_WIDTH, 0, LCD_HEIGHT, DisplayBuffer);
        xSemaphoreGive(display_mutex);
    }
}

// 截图任务
void image_shot_task(void *param)
{
    static uint32_t value_mask = 0;
    while(1)
    {
        xTaskNotifyWait(0xFFFFFFFF,0xFFFFFFFF, &value_mask, portMAX_DELAY);
        if(value_mask)  // 判断sd卡是否存在
        {
            if(image_idx < 999)
            {
                image_idx++;
            }
            else
            {
                image_idx = 1;
            }
            write_data(IMAGE_KEY_INDEX, image_idx);

            xSemaphoreTake(image_mutex, portMAX_DELAY);
            if (CurrentRGBValueMatrix != NULL) 
            {
                screenshot(image_idx, CurrentRGBValueMatrix, IMAGE_WIDTH, IMAGE_HEIGHT);
            }
            xSemaphoreGive(image_mutex);
            DrawTitle(CAPTURE_SUCCESSFUL_FLAG);
        }
        else
        {
            DrawTitle(NO_SD_CARD_FLAG);
        }
    }
}

// 单次测距任务
void ranging_task(void *param)
{
    while(1)
    {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        // 需要测距时
        int16_t distance_mm;
        uint8_t status;   
        
        if (vl53l1x_single_ranging(&distance_mm, &status, 200)) 
        {
            if (status == 6 || status == 0) 
            {
                DrawTitle(distance_mm);
            } 
            else 
            {
                DrawTitle(RANGE_INVALID_FLAG);
            }
        } 
        else 
        {
            DrawTitle(RANGE_ERROR_FLAG);
        }
    }
}

// 电池电压测量任务
void battery_voltage_task(void *param)
{
    while(1)
    {
        voltage_level = one_shot_read();
        Fill_battery_frame(voltage_level);
        vTaskDelay(pdMS_TO_TICKS(3000)); // 间隔3s采样一次
    }
}

// 按键轮询处理任务
void button_polling_task(void *param) 
{
    s_keys[0].pin = TRIGGER_KEY_PIN;
    s_keys[1].pin = DOWN_KEY_PIN;
    s_keys[2].pin = UP_KEY_PIN;
    s_keys[3].pin = RIGHT_KEY_PIN;
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(1) 
    {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10)); // 精确每 10ms 运行一次
        uint64_t now_ms = get_tick_ms(); // 64位，永不溢出
        
        for(uint8_t i = 0; i < 4; i++)
        {
            //传递状态的值，七六五位用于传递按键是什么状态（高位往下：单双长击），最后四位用于传递引脚（高位往下：右上下前）
            uint32_t value_mask = 0x0;
            key_handle_t *key = &s_keys[i];
            bool level = gpio_get_level(key->pin); // 假设按下为低电平(0)，释放为高(1)
            bool is_pressed = (level == 0);

            value_mask |= 1 << i;

            switch (key->state) 
            {
                case KEY_STATE_IDLE:
                    // 复位事件处理标志，准备处理新动作
                    key->is_event_handled = false;
                    if (is_pressed) 
                    {
                        // 记录首次检测到按下的时间
                        key->last_tick_ms = now_ms;
                        key->state = KEY_STATE_DEBOUNCE;
                    }
                    break;

                case KEY_STATE_DEBOUNCE:
                    if (is_pressed) 
                    {
                        // 消抖确认：如果持续按下超过 50ms
                        if ((now_ms - key->last_tick_ms) >= KEY_DEBOUNCE_MS) 
                        {
                            // 记录真正按下的时间点
                            key->press_tick_ms = now_ms;
                            key->is_long_triggered = false;
                            key->state = KEY_STATE_PRESSED;
                        }
                    } 
                    else 
                    {
                        // 未到消抖时间就释放了，视为抖动，回到 IDLE
                        key->state = KEY_STATE_IDLE;
                    }
                    break;

                case KEY_STATE_PRESSED:
                    if (is_pressed) 
                    {
                        // 长按检测（只触发一次）
                        if (!key->is_long_triggered && (now_ms - key->press_tick_ms) >= KEY_LONG_PRESS_MS) 
                        {
                            key->is_long_triggered = true;
                            key->is_event_handled = true; // 锁定，禁止后续单击/双击

                            value_mask |= LONG_MASK;
                            xTaskNotify(button_task_handle, value_mask, eSetValueWithOverwrite);
                            //ESP_LOGI(TAG, "Long pressed!");
                        }
                    } 
                    else 
                    {  
                        // 如果已经触发了长按，则释放时什么都不做
                        if (key->is_long_triggered) 
                        {
                            key->state = KEY_STATE_IDLE;
                            break;
                        }
                        
                        // 短按处理（判断双击）
                        if (key->last_click_tick_ms != 0 && (now_ms - key->last_click_tick_ms) < KEY_DOUBLE_CLICK_MS) 
                        {
                            // 触发双击，立即锁定，防止残留单击
                            if (!key->is_event_handled) 
                            {
                                key->is_event_handled = true;

                                value_mask |= DOUBLE_MASK;
                                xTaskNotify(button_task_handle, value_mask, eSetValueWithOverwrite);
                                //ESP_LOGI(TAG, "Double click!");
                            }
                            key->last_click_tick_ms = 0;
                            key->state = KEY_STATE_IDLE;
                        } 
                        else 
                        {
                            // 第一次单击，记录时间，进入等待双击超时状态
                            key->last_click_tick_ms = now_ms;
                            key->state = KEY_STATE_WAIT_DOUBLE;
                            // 注意：此时不触发单击回调，等待超时或第二次按下
                        }
                    }
                    break;

                    case KEY_STATE_WAIT_DOUBLE:
                    if (is_pressed) 
                    {
                        // 等待期间按下了，重新开始消抖（可能是第二次点击的开始）
                        key->last_tick_ms = now_ms;
                        key->state = KEY_STATE_DEBOUNCE;
                    } 
                    else 
                    {
                        // 检查等待双击是否超时
                        if ((now_ms - key->last_click_tick_ms) >= KEY_DOUBLE_CLICK_MS) 
                        {
                            // 只有未被处理（未触发双击或长按）时，才触发单击
                            if (!key->is_event_handled) 
                            {
                                key->is_event_handled = true;

                                value_mask |= SINGLE_MASK;
                                xTaskNotify(button_task_handle, value_mask, eSetValueWithOverwrite);
                                //ESP_LOGI(TAG,"Single pressed!");
                            }
                            key->last_click_tick_ms = 0;
                            key->state = KEY_STATE_IDLE;
                        }
                    }
                    break;
            }
        }
    }
}

// 按键处理任务
void button_processing_task(void *param) 
{
    static uint32_t value_mask; 
    static uint8_t button, status;
    while(1)
    {
        xTaskNotifyWait(0xFFFFFFFF,0xFFFFFFFF,&value_mask,portMAX_DELAY);
        button = (uint8_t)(value_mask & 0Xf);
        status = (uint8_t)(value_mask & 0X70);

        switch (button)
        {
            case TRIGGER_KEY_MASK:
                if(status == SINGLE_MASK)
                {
                    #ifdef VL53L1X
                    xTaskNotifyGive(ranging_task_handle); // 给测距任务发通知
                    #endif
                    
                    buzzer_beep_short();
                }
                else if(status == DOUBLE_MASK)
                {
                    buzzer_beep_twice();
                }
                else
                {
                    xTaskNotify(image_shot_task_handle, SD_card_exist, eSetValueWithOverwrite);
                    buzzer_beep_long();
                }
                break;
            case DOWN_KEY_MASK:
                if(status == SINGLE_MASK)
                {
                    menu_move_down();
                    buzzer_beep_short();
                }
                else if(status == DOUBLE_MASK)
                {
                    buzzer_beep_twice();
                }
                else
                {
                    buzzer_beep_long();
                }
                break;
            case UP_KEY_MASK:
                if(status == SINGLE_MASK)
                {
                    menu_move_up();
                    buzzer_beep_short();
                }
                else if(status == DOUBLE_MASK)
                {
                    buzzer_beep_twice();
                }
                else
                {
                    buzzer_beep_long();
                }
                break;
            case RIGHT_KEY_MASK:
                if(status == SINGLE_MASK)
                {
                    menu_enter_child();
                    buzzer_beep_short();
                }
                else if(status == DOUBLE_MASK)
                {
                    buzzer_beep_twice();
                }
                else
                {
                    menu_goto_parent();
                    buzzer_beep_long();
                }
                break;
        }
    }
}

void UI_refresh_task(void *param)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(1)
    {
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(30));
        MenuScrollUpdate();   // 步进动画
        MenuScrollDraw();     // 绘制显示区域
        DrawTemperature(hot_temp, cold_temp, temp_units_type);
    }
}

void Factory_Reset(void)
{
    buzzer_level = ON;
    interpolation_method = ON;
    cold_crossing = ON;
    hot_crossing = ON;
    scheme_idx = SCHEME_PSEUDO1_INDEX;
    refreshrate_idx = RefreshRate_16Hz_idx;
    switch (scheme_idx)
    {
        case SCHEME_GRAY_INDEX:
            Set_Color_Scheme(COLOR_SCHEME_GRAY);
            break;
        case SCHEME_METAL_INDEX:
            Set_Color_Scheme(COLOR_SCHEME_METAL);
            break;
        case SCHEME_PSEUDO1_INDEX:
            Set_Color_Scheme(COLOR_SCHEME_PSEUDO1);
            break;
        case SCHEME_PSEUDO2_INDEX:
            Set_Color_Scheme(COLOR_SCHEME_PSEUDO2);
            break;
        case SCHEME_RAINBOW1_INDEX:
            Set_Color_Scheme(COLOR_SCHEME_RAINBOW1);
            break;
        case SCHEME_RAINBOW2_INDEX:
            Set_Color_Scheme(COLOR_SCHEME_RAINBOW2);
            break;
    }
    switch (refreshrate_idx)
    {
        case RefreshRate_2Hz_idx:
            Set_RefreshRate_2Hz();
            break;
        case RefreshRate_4Hz_idx:
            Set_RefreshRate_4Hz();
            break;
        case RefreshRate_8Hz_idx:
            Set_RefreshRate_8Hz();
            break;
        case RefreshRate_16Hz_idx:
            Set_RefreshRate_16Hz();
            break;
        case RefreshRate_32Hz_idx:
            Set_RefreshRate_32Hz();
            break;
    }
    write_data(BUZZER_KEY_INDEX, buzzer_level);
    write_data(INTERPOLATION_KEY_INDEX, interpolation_method);
    write_data(COLD_KEY_INDEX, cold_crossing);
    write_data(HOT_KEY_INDEX, hot_crossing);
    write_data(SCHEME_KEY_INDEX, scheme_idx);
    write_data(REFRESHRATE_KEY_INDEX, refreshrate_idx);
}