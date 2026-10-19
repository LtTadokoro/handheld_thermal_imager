#include "gpio.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include "task_management.h"
#include "SD_card.h"

//BUZZER引脚为7，使用通道0，频率设为4kHz
#define BUZZER_PIN          GPIO_NUM_7
#define BUZZER_CHANNEL      LEDC_CHANNEL_0
#define BUZZER_FREQ         4000
#define BUZZER_DUTY         50

static const char *TAG = "GPIO";

uint8_t buzzer_level = 1;   // 默认开启
uint8_t laser_level = 0;    // 默认关闭
uint8_t SD_card_exist = 0;  // 默认没插卡

void turn_laser(uint8_t laser_level)
{
    gpio_set_level(LASER_PIN, laser_level);
}

uint8_t detect_SD_card(void)
{
    if(gpio_get_level(SD_DETECT_PIN) == 0)
    {
        SD_card_exist = 1;
    }
    else
    {
        SD_card_exist = 0;
    }
    return SD_card_exist;
}

void buzzer_beep_short(void)
{
    if(buzzer_level != 1)
    {
        return;
    }
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);
    vTaskDelay(pdMS_TO_TICKS(80));
    ledc_stop(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 0);
}

void buzzer_beep_long(void)
{
    if(buzzer_level != 1)
    {
        return;
    }
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);
    vTaskDelay(pdMS_TO_TICKS(250));
    ledc_stop(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 0);
}

void buzzer_beep_twice(void)
{
    if(buzzer_level != 1)
    {
        return;
    }
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);
    vTaskDelay(pdMS_TO_TICKS(80));
    ledc_stop(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL);
    vTaskDelay(pdMS_TO_TICKS(80));
    ledc_stop(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 0);
}

void gpio_init(void)
{
    gpio_config_t right_key;
    right_key.pin_bit_mask = BIT64(RIGHT_KEY_PIN);
    right_key.mode = GPIO_MODE_INPUT;
    right_key.pull_up_en = GPIO_PULLUP_DISABLE;
    right_key.pull_down_en = GPIO_PULLDOWN_DISABLE;
    right_key.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&right_key);

    gpio_config_t up_key;
    up_key.pin_bit_mask = BIT64(UP_KEY_PIN);
    up_key.mode = GPIO_MODE_INPUT;
    up_key.pull_up_en = GPIO_PULLUP_DISABLE;
    up_key.pull_down_en = GPIO_PULLDOWN_DISABLE;
    up_key.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&up_key);

    gpio_config_t down_key;
    down_key.pin_bit_mask = BIT64(DOWN_KEY_PIN);
    down_key.mode = GPIO_MODE_INPUT;
    down_key.pull_up_en = GPIO_PULLUP_DISABLE;
    down_key.pull_down_en = GPIO_PULLDOWN_DISABLE;
    down_key.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&down_key);

    gpio_config_t trigger_key;
    trigger_key.pin_bit_mask = BIT64(TRIGGER_KEY_PIN);
    trigger_key.mode = GPIO_MODE_INPUT;
    trigger_key.pull_up_en = GPIO_PULLUP_DISABLE;
    trigger_key.pull_down_en = GPIO_PULLDOWN_DISABLE;
    trigger_key.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&trigger_key);

    gpio_config_t sd_pin;
    sd_pin.pin_bit_mask = BIT64(SD_DETECT_PIN);
    sd_pin.mode = GPIO_MODE_INPUT;
    sd_pin.pull_up_en = GPIO_PULLUP_DISABLE;
    sd_pin.pull_down_en = GPIO_PULLDOWN_DISABLE;
    sd_pin.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&sd_pin);

    gpio_config_t laser;
    laser.pin_bit_mask = BIT64(LASER_PIN);
    laser.mode = GPIO_MODE_OUTPUT;
    laser.pull_up_en = GPIO_PULLUP_DISABLE;
    laser.pull_down_en = GPIO_PULLDOWN_DISABLE;
    laser.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&laser);
    gpio_set_level(LASER_PIN, 0);


    // 配置LEDC定时器（决定PWM频率和分辨率）
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,       // 使用低速模式（推荐用于GPIO较低频率）
        .timer_num = LEDC_TIMER_0,               // 使用定时器0
        .duty_resolution = LEDC_TIMER_10_BIT,    // 设置占空比精度为10位（1024级）
        .freq_hz = BUZZER_FREQ,                      // 设置PWM频率
        .clk_cfg = LEDC_AUTO_CLK,                // 自动选择时钟源
    };
    // 应用定时器配置
    ledc_timer_config(&timer_conf);
    // 配置LEDC通道
    ledc_channel_config_t ch_conf = {
        .channel = BUZZER_CHANNEL,                      // 使用的LEDC通道
        .duty = BUZZER_DUTY,                        // 初始占空比为0（10位值）
        .gpio_num = BUZZER_PIN,                         // 绑定的GPIO引脚
        .speed_mode = LEDC_LOW_SPEED_MODE,       // 使用低速模式
        .hpoint = 0,                              // 高电平起始点，默认从0开始
        .timer_sel = LEDC_TIMER_0,               // 使用的定时器编号
        .intr_type = LEDC_INTR_DISABLE           // 禁用中断
    };
    // 应用通道配置
    ledc_channel_config(&ch_conf);
    ledc_stop(LEDC_LOW_SPEED_MODE, BUZZER_CHANNEL, 0);
}