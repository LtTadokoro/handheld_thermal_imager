#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"
#include "vl53l1x.h"
#include "buffer.h"
#include "display.h"
#include "task_management.h"
#include "gpio.h"
#include "SD_card.h"
#include "adc.h"
#include "ST7789_driver.h"
#include "UI.h"
#include "menu.h"
#include "nvs_wr.h"

#include "config.h"


static const char *TAG = "MAIN";

void app_main(void)
{
    init_sync_objects();                    // 创建同步对象
    nvs_Init();                             // nvs初始化
    read_data();                            // 从flash中读取标志位
    ADC_Init();                             // adc初始化
    alloc_psram_buffers();                  // 缓冲区初始化
    lcd_init();                             // lcd屏幕初始化
    Fill_Black_Background(DisplayBuffer);   // 屏幕填充黑色背景
    Color_Scheme_Init();                    // 色彩表初始化
    UI_Init();                              // 边角ui初始化
    gpio_init();                            // gpio初始化
    SD_card_Init();                         // sd卡初始化（如果存在）
    Menu_Init();                            // 显示菜单初始化
    MLX90640_I2CInit();                     // 传感器i2c初始化

    #ifdef VL53L1X
    vl53l1x_I2CInit();                      // 测距模块i2c初始化
    #endif

    vTaskDelay(pdMS_TO_TICKS(100));
    MLX90640_Init();                        // 传感器初始化

    #ifdef VL53L1X
    vl53l1xInit();                          // 测距模块初始化
    #endif

    vTaskDelay(pdMS_TO_TICKS(50));
    turn_on_display();                      // 打开屏幕显示        
    
    xTaskCreate(battery_voltage_task, "battery_voltage_task", 4096, NULL, 5, NULL);             // adc测量任务
    xTaskCreate(read_task, "read_task", 4096, NULL, 14, NULL);                                  // 读取任务
    xTaskCreate(transform_task, "transform_task", 8192, NULL, 14, NULL);                        // 转换任务
    xTaskCreate(UI_refresh_task, "UI_refresh_task", 4096, NULL, 11, NULL);                      // ui刷新写入任务
    xTaskCreate(display_task, "display_task", 4096, NULL, 13, NULL);                            // 显示刷新任务
    xTaskCreate(button_polling_task, "polling_task", 2048, NULL, 10, &polling_task_handle);     // 按键轮询任务
    xTaskCreate(button_processing_task, "button_task", 2048, NULL, 10, &button_task_handle);    // 按键处理任务，由轮询任务唤起
    xTaskCreate(image_shot_task, "image_shot_task", 8192, NULL, 10, &image_shot_task_handle);   // 截图任务

    #ifdef VL53L1X
    xTaskCreate(ranging_task, "ranging_task", 8192, NULL, 15, &ranging_task_handle);            // 测距任务
    #endif
}
