#include "adc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>

const static char *TAG = "ADC";

#define ADC_CHANNEL         ADC_CHANNEL_5
#define ADC_UNIT            ADC_UNIT_1
#define ADC_ATTENUATION     ADC_ATTEN_DB_2_5
#define ADC_BITWIDTH        ADC_BITWIDTH_12

#define FULL_MV             975
#define MEDIUM_MV           960
#define HALF_MV             940
#define LOW_MV              925

adc_oneshot_unit_handle_t adc1_handle = NULL;
adc_cali_handle_t adc1_cali_chan5_handle = NULL;
int adc_raw, voltage;

static bool ADC_Calibration_Init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

    if (!calibrated) 
    {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) 
        {
            calibrated = true;
        }
    }

    *out_handle = handle;
    if (ret == ESP_OK) 
    {
        ESP_LOGI(TAG, "Calibration Success");
    } 
    else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) 
    {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } 
    else 
    {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static void example_adc_calibration_deinit(adc_cali_handle_t handle)
{
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));
}

void ADC_Init(void)
{
    //adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTENUATION,
        .bitwidth = ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL, &config));

    ADC_Calibration_Init(ADC_UNIT, ADC_CHANNEL, ADC_ATTENUATION, &adc1_cali_chan5_handle);
}

uint8_t one_shot_read(void)
{
    uint16_t voltage_level;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_CHANNEL, &adc_raw));
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan5_handle, adc_raw, &voltage));
    if (voltage >= FULL_MV)
    {
        voltage_level = 4;
    }
    else if (voltage >= MEDIUM_MV && voltage < FULL_MV)
    {
        voltage_level = 3;
    }
    else if (voltage >= HALF_MV && voltage < MEDIUM_MV)
    {
        voltage_level = 2;
    }
    else
    {
        voltage_level = 1;
    }
    return voltage_level;
}