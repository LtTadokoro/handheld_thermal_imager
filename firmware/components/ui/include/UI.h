#ifndef _UI_H_
#define _UI_H_

#include <stdio.h>

#define RANGE_INVALID_FLAG      -1
#define RANGE_ERROR_FLAG        -2
#define CAPTURE_SUCCESSFUL_FLAG -3
#define NO_SD_CARD_FLAG         -4

void MenuScrollTo(char* new_text, int direction);
void MenuScrollUpdate(void);
void MenuScrollDraw(void);
void Fill_battery_frame(uint8_t voltage_level);
void DrawTitle(int16_t value);
void DrawTemperature(float upper_temp, float lower_temp, uint8_t temp_type);
void UI_Init(void);

#endif