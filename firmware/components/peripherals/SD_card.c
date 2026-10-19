#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>
#include <driver/spi_common.h>
#include <driver/spi_master.h>
#include "SD_card.h"
#include "image_processing.h"
#include "gpio.h"

#define SD_CS       GPIO_NUM_14
#define SD_MOSI     GPIO_NUM_21
#define SD_CLK      GPIO_NUM_47
#define SD_MISO     GPIO_NUM_48

static const char *TAG = "SD_TEST";

uint16_t image_idx = 0;  // 热像图截图编号，初始为0，实际初次截图时为1，最大为999

static inline void rgb565_to_rgb888(uint16_t rgb565, uint8_t *r, uint8_t *g, uint8_t *b) 
{
    // 如果颜色值已经交换过字节（低字节在前），先交换回来
    uint16_t color = (rgb565 >> 8) | (rgb565 << 8);
    *r = (color >> 11) & 0x1F;
    *r = (*r << 3) | (*r >> 2);
    *g = (color >> 5) & 0x3F;
    *g = (*g << 2) | (*g >> 4);
    *b = color & 0x1F;
    *b = (*b << 3) | (*b >> 2);
}

static esp_err_t write_bmp(const char *path, uint16_t *image, int w, int h) 
{
    FILE *f = fopen(path, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", path);
        return ESP_FAIL;
    }

    int row_size = (w * 3 + 3) & ~3;
    int image_size = row_size * h;

    uint8_t bmp_header[14] = {
        0x42, 0x4D,
        0, 0, 0, 0,
        0, 0, 0, 0,
        54, 0, 0, 0
    };
    uint8_t info_header[40] = {
        40, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        1, 0,
        24, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0
    };

    uint32_t width = w;
    uint32_t height = h;
    memcpy(info_header + 4,  &width,  4);
    memcpy(info_header + 8,  &height, 4);
    uint32_t file_size = 54 + image_size;
    memcpy(bmp_header + 2, &file_size, 4);
    memcpy(info_header + 20, &image_size, 4);

    fwrite(bmp_header, 1, 14, f);
    fwrite(info_header, 1, 40, f);

    uint8_t *row_buf = malloc(row_size);
    if (!row_buf) {
        fclose(f);
        ESP_LOGE(TAG, "No memory for row buffer");
        return ESP_ERR_NO_MEM;
    }
    memset(row_buf, 0, row_size);

    for (int y = h - 1; y >= 0; y--) {
        uint8_t *dst = row_buf;
        for (int x = 0; x < w; x++) {
            uint16_t px = image[y * w + x];
            uint8_t r, g, b;
            rgb565_to_rgb888(px, &r, &g, &b);
            *dst++ = b;
            *dst++ = g;
            *dst++ = r;
        }
        fwrite(row_buf, 1, row_size, f);
    }

    free(row_buf);
    fclose(f);
    ESP_LOGI(TAG, "BMP written successfully: %s", path);
    return ESP_OK;
}
/*
//挂载sd卡
void mount_sdcard(sdmmc_host_t host, sdspi_device_config_t slot_config, esp_vfs_fat_mount_config_t mount_config, sdmmc_card_t *card)
{
    int ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "SD card mounted, capacity: %llu MB", (uint64_t)card->csd.capacity * card->csd.sector_size / (1024 * 1024));
}

//卸载sd卡
void umount_sdcard(sdmmc_card_t *card)
{
    esp_err_t umount_err = esp_vfs_fat_sdcard_unmount("/sdcard", card);
    if (umount_err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to unmount SD card: %s", esp_err_to_name(umount_err));
    } 
    else 
    {
        ESP_LOGI(TAG, "SD card unmounted successfully.");
    }
}
    */

void screenshot(uint8_t image_idx, uint16_t *image_matrix, uint16_t image_width, uint16_t image_height)
{
    /*
    char path[] = "/sdcard/image000.bmp";
    snprintf(text, sizeof(text), "captured: %03u", (unsigned)image_idx);
    path[13] = (image_idx / 100) % 10 + 48;
    path[14] = (image_idx / 10) % 10  + 48;
    path[15] = image_idx % 10  + 48;
    */

    char path[22];
    snprintf(path, sizeof(path), "/sdcard/image%03u.bmp", (unsigned)image_idx);

    esp_err_t ret = write_bmp(path, image_matrix, image_width, image_height);
    if (ret == ESP_OK) 
    {
        ESP_LOGI(TAG, "Test completed successfully. File %s created.",path);
    } 
    else
    {
        ESP_LOGE(TAG, "Write failed.");
    }
}

void SD_card_Init(void)
{
    // 先检测SD卡是否存在
    if(detect_SD_card() != 1)
    {
        ESP_LOGE(TAG, "No SD card!");
        return;
    }
    ESP_LOGI(TAG, "Initializing SD card via SPI3...");
    // 初始化 SPI3 总线
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_MOSI,
        .miso_io_num = SD_MISO,
        .sclk_io_num = SD_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    esp_err_t ret = spi_bus_initialize(SPI3_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI3 bus: %s", esp_err_to_name(ret));
        return;
    }

    // 配置 SD 卡主机
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI3_HOST;
    host.max_freq_khz = 20000;

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.host_id = SPI3_HOST;
    slot_config.gpio_cs = SD_CS;
    slot_config.gpio_cd = GPIO_NUM_NC;
    slot_config.gpio_wp = GPIO_NUM_NC;

    // 挂载配置
    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t *card;
    
    ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to mount SD card: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "SD card mounted, capacity: %llu MB", (uint64_t)card->csd.capacity * card->csd.sector_size / (1024 * 1024));
}