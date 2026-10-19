#ifndef _SD_CARD_H_
#define _SD_CARD_H_

#include <stdio.h>

extern uint16_t image_idx;

void screenshot(uint8_t image_idx, uint16_t *image_matrix, uint16_t image_width, uint16_t image_height);
void SD_card_Init(void);

#endif