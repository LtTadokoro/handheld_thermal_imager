#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "MLX90640_I2C_Driver.h"
#include <driver/i2c_master.h>

static const char *TAG = "I2C0";

// 静态 I2C 句柄
static i2c_master_bus_handle_t  bus_handle  = NULL;
static i2c_master_dev_handle_t  dev_handle  = NULL;

static uint8_t  g_slave_addr = MLX90640_DEFAULT_ADDR;

// ---------------------------------------------------------------------
// I2C 底层接口实现 (直接调用 ESP-IDF 驱动)
// ---------------------------------------------------------------------
void MLX90640_I2CInit(void)
{
    ESP_LOGI(TAG, "Initializing I2C master bus...");
    i2c_master_bus_config_t bus_config = {
        .i2c_port = MLX90640_I2C_NUM,
        .sda_io_num = MLX90640_I2C_SDA_IO,
        .scl_io_num = MLX90640_I2C_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = g_slave_addr,
        .scl_speed_hz = MLX90640_I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));
    ESP_LOGI(TAG, "I2C master bus initialized (address 0x%02X, freq %d Hz)",
             g_slave_addr, MLX90640_I2C_FREQ_HZ);
}

int MLX90640_I2CGeneralReset(void)
{
    ESP_LOGI(TAG, "Performing general call reset...");
    uint8_t reset_cmd[2] = {0x00, 0x06};
    esp_err_t ret = i2c_master_transmit(dev_handle, reset_cmd, sizeof(reset_cmd),
                                        MLX90640_I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "General reset failed: %s", esp_err_to_name(ret));
        return -MLX90640_I2C_NACK_ERROR;
    }
    return MLX90640_NO_ERROR;
}

int MLX90640_I2CRead(uint8_t slaveAddr, uint16_t startAddress, uint16_t nMemAddressRead, uint16_t *data)
{
    if (slaveAddr != g_slave_addr) {
        ESP_LOGW(TAG, "Slave address mismatch (0x%02X vs 0x%02X), updating...", slaveAddr, g_slave_addr);
        g_slave_addr = slaveAddr;
        // 重新创建设备
        i2c_master_bus_rm_device(dev_handle);
        i2c_device_config_t dev_config = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = g_slave_addr,
            .scl_speed_hz = MLX90640_I2C_FREQ_HZ,
        };
        ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));
    }

    uint8_t reg_addr[2];
    reg_addr[0] = (uint8_t)(startAddress >> 8);
    reg_addr[1] = (uint8_t)(startAddress & 0x00FF);
    // 读取 nMemAddressRead 个 word (每个 word 2 字节)
    esp_err_t ret = i2c_master_transmit_receive(dev_handle, reg_addr, 2,
                                                (uint8_t *)data, nMemAddressRead * 2,
                                                MLX90640_I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C read failed: %s", esp_err_to_name(ret));
        return -MLX90640_I2C_NACK_ERROR;
    }
    // 数据是大端模式 (MSB first)，但 API 要求小端，需要交换字节
    for (int i = 0; i < nMemAddressRead; i++) {
        uint16_t temp = data[i];
        data[i] = ((temp & 0xFF) << 8) | ((temp >> 8) & 0xFF);
    }
    return MLX90640_NO_ERROR;
}

int MLX90640_I2CWrite(uint8_t slaveAddr, uint16_t writeAddress, uint16_t data)
{
    if (slaveAddr != g_slave_addr) {
        // 类似读操作中的地址更新
        MLX90640_I2CRead(slaveAddr, writeAddress, 1, &data); // 临时调用一次以更新地址
    }

    uint8_t write_buf[4];
    write_buf[0] = (uint8_t)(writeAddress >> 8);
    write_buf[1] = (uint8_t)(writeAddress & 0x00FF);
    // 数据也需要交换字节 (API 传入的小端 -> 大端)
    //uint16_t data_be = ((data & 0x00FF) << 8) | ((data >> 8) & 0x00FF);
    write_buf[2] = (uint8_t)(data >> 8);
    write_buf[3] = (uint8_t)(data & 0x00FF);

    esp_err_t ret = i2c_master_transmit(dev_handle, write_buf, sizeof(write_buf),
                                        MLX90640_I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C write failed: %s", esp_err_to_name(ret));
        return -MLX90640_I2C_NACK_ERROR;
    }
    return MLX90640_NO_ERROR;
}

void MLX90640_I2CFreqSet(int freq)
{
    ESP_LOGI(TAG, "I2C frequency change requested: %d kHz. (Not dynamically changed, reconfigure needed)", freq);
    // 如果需要动态修改频率，可以重新初始化 I2C，但一般不需要。
}