#ifndef _TASK_MANAGEMENT_H_
#define _TASK_MANAGEMENT_H_

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


extern TaskHandle_t polling_task_handle;
extern TaskHandle_t button_task_handle;
extern TaskHandle_t image_shot_task_handle;
extern TaskHandle_t ranging_task_handle;

void read_task(void *param);
void transform_task(void *param);
void display_task(void *param);
void button_polling_task(void *pvParameters);
void button_processing_task(void *param);
void image_shot_task(void *param);
void ranging_task(void *param);
void battery_voltage_task(void *param);
void UI_refresh_task(void *param);
void Factory_Reset(void);

#endif