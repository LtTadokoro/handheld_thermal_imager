#ifndef _NVS_WR_H_
#define _NVS_WR_H_

#include <stdio.h>

#define BUZZER_KEY_INDEX          0
#define INTERPOLATION_KEY_INDEX   1
#define COLD_KEY_INDEX            2
#define HOT_KEY_INDEX             3
#define SCHEME_KEY_INDEX          4
#define REFRESHRATE_KEY_INDEX     5
#define TEMP_UNITS_KEY_INDEX      6
#define IMAGE_KEY_INDEX           7

void nvs_Init(void);
void read_data(void);
void write_data(uint8_t idx, uint8_t val);

#endif