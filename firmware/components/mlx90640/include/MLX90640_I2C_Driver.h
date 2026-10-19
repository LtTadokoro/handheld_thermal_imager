#ifndef _MLX90640_I2C_Driver_H_
#define _MLX90640_I2C_Driver_H_

#include <stdint.h>
#include "MLX90640_API.h"

// ---------------------------------------------------------------------
// 用户可配置的硬件接口宏 (根据你的实际电路修改)
// ---------------------------------------------------------------------
#ifndef MLX90640_I2C_NUM
#define MLX90640_I2C_NUM            I2C_NUM_0
#endif
#ifndef MLX90640_I2C_SDA_IO
#define MLX90640_I2C_SDA_IO         GPIO_NUM_17      // GPIO number for SDA
#endif
#ifndef MLX90640_I2C_SCL_IO
#define MLX90640_I2C_SCL_IO         GPIO_NUM_18      // GPIO number for SCL
#endif
#ifndef MLX90640_I2C_FREQ_HZ
#define MLX90640_I2C_FREQ_HZ        1000000 // 1mHz, can be up to 1MHz
#endif
#ifndef MLX90640_I2C_TIMEOUT_MS
#define MLX90640_I2C_TIMEOUT_MS     1000
#endif
#ifndef MLX90640_DEFAULT_ADDR
#define MLX90640_DEFAULT_ADDR       0x33   // Default I2C slave address
#endif

void MLX90640_I2CInit(void);
int MLX90640_I2CGeneralReset(void);
int MLX90640_I2CRead(uint8_t slaveAddr,uint16_t startAddress, uint16_t nMemAddressRead, uint16_t *data);
int MLX90640_I2CWrite(uint8_t slaveAddr,uint16_t writeAddress, uint16_t data);
void MLX90640_I2CFreqSet(int freq);

#endif