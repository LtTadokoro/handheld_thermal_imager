#ifndef _GPIO_H_
#define _GPIO_H_

#include <stdio.h>

#define RIGHT_KEY_PIN       GPIO_NUM_39
#define UP_KEY_PIN          GPIO_NUM_40
#define DOWN_KEY_PIN        GPIO_NUM_41
#define TRIGGER_KEY_PIN     GPIO_NUM_42
#define LASER_PIN           GPIO_NUM_5
#define SD_DETECT_PIN       GPIO_NUM_38

void turn_laser(uint8_t laser_level);
uint8_t detect_SD_card(void);
void buzzer_beep_short(void);
void buzzer_beep_long(void);
void buzzer_beep_twice(void);
void gpio_init(void);

extern uint8_t buzzer_level;
extern uint8_t laser_level;
extern uint8_t SD_card_exist;

#endif