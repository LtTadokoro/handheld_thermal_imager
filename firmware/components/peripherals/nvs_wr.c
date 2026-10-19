#include "nvs_wr.h"
#include "MLX90640_API.h"
#include "gpio.h"
#include "image_processing.h"
#include "task_management.h"
#include "SD_card.h"
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_log.h>

static const char *TAG = "NVS";

void nvs_Init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void read_data(void)
{
    nvs_handle_t my_handle;
    esp_err_t err;
    uint8_t val;

    // 只读方式打开，若命名空间不存在会返回 ESP_ERR_NVS_NOT_FOUND
    err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) 
    {
        ESP_LOGI(TAG, "No NVS data, use default values");
        return;
    }
    if (err != ESP_OK) 
    {
        ESP_LOGW(TAG, "nvs_open failed: %s", esp_err_to_name(err));
        return;
    }

    // key0 -> g_buzzer_key
    err = nvs_get_u8(my_handle, "key0", &val);
    if (err == ESP_OK) 
    {
        buzzer_level = val;
    } 
    else if (err != ESP_ERR_NVS_NOT_FOUND) 
    {
        ESP_LOGW(TAG, "read key0 failed: %s", esp_err_to_name(err));
    }

    // key1 -> g_interpolation_key
    err = nvs_get_u8(my_handle, "key1", &val);
    if (err == ESP_OK) 
    {
        interpolation_method = val;
    } 
    else if (err != ESP_ERR_NVS_NOT_FOUND) 
    {
        ESP_LOGW(TAG, "read key1 failed: %s", esp_err_to_name(err));
    }

    // key2 -> g_cold_key
    err = nvs_get_u8(my_handle, "key2", &val);
    if (err == ESP_OK) 
    {
        cold_crossing = val;
    } 
    else if (err != ESP_ERR_NVS_NOT_FOUND) 
    {
        ESP_LOGW(TAG, "read key2 failed: %s", esp_err_to_name(err));
    }

    // key3 -> g_hot_key
    err = nvs_get_u8(my_handle, "key3", &val);
    if (err == ESP_OK) 
    {
        hot_crossing = val;
    } 
    else if (err != ESP_ERR_NVS_NOT_FOUND) 
    {
        ESP_LOGW(TAG, "read key3 failed: %s", esp_err_to_name(err));
    }

    // key4 -> g_scheme_key
    err = nvs_get_u8(my_handle, "key4", &val);
    if (err == ESP_OK) 
    {
        scheme_idx = val;
    } 
    else if (err != ESP_ERR_NVS_NOT_FOUND) 
    {
        ESP_LOGW(TAG, "read key4 failed: %s", esp_err_to_name(err));
    }

    // key5 -> g_refreshrate_key
    err = nvs_get_u8(my_handle, "key5", &val);
    if (err == ESP_OK) 
    {
        refreshrate_idx = val;
    } 
    else if (err != ESP_ERR_NVS_NOT_FOUND) 
    {
        ESP_LOGW(TAG, "read key5 failed: %s", esp_err_to_name(err));
    }

    // key6 -> g_temp_units_key
    err = nvs_get_u8(my_handle, "key6", &val);
    if (err == ESP_OK) 
    {
        temp_units_type = val;
    } 
    else if (err != ESP_ERR_NVS_NOT_FOUND) 
    {
        ESP_LOGW(TAG, "read key6 failed: %s", esp_err_to_name(err));
    }

    // key7 -> g_image_key
    err = nvs_get_u8(my_handle, "key7", &val);
    if (err == ESP_OK) 
    {
        image_idx = val;
    } 
    else if (err != ESP_ERR_NVS_NOT_FOUND) 
    {
        ESP_LOGW(TAG, "read key6 failed: %s", esp_err_to_name(err));
    }

    nvs_close(my_handle);
}

void write_data(uint8_t idx, uint8_t val)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) return;

    switch(idx)
    {
        case BUZZER_KEY_INDEX:
            nvs_set_u8(my_handle, "key0", val);
            break;
        case INTERPOLATION_KEY_INDEX:
            nvs_set_u8(my_handle, "key1", val);
            break;
        case COLD_KEY_INDEX:
            nvs_set_u8(my_handle, "key2", val);
            break;
        case HOT_KEY_INDEX:
            nvs_set_u8(my_handle, "key3", val);
            break;
        case SCHEME_KEY_INDEX:
            nvs_set_u8(my_handle, "key4", val);
            break;
        case REFRESHRATE_KEY_INDEX:
            nvs_set_u8(my_handle, "key5", val);
            break;
        case TEMP_UNITS_KEY_INDEX:
            nvs_set_u8(my_handle, "key6", val);
            break;
        case IMAGE_KEY_INDEX:
            nvs_set_u8(my_handle, "key7", val);
            break;
    }
    err = nvs_commit(my_handle);
    nvs_close(my_handle);
}