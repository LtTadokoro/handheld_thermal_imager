/*******************************************************************************
 Copyright (C) 2016, STMicroelectronics International N.V.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright
 notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.
 * Neither the name of STMicroelectronics nor the
 names of its contributors may be used to endorse or promote products
 derived from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 NON-INFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS ARE DISCLAIMED.
 IN NO EVENT SHALL STMICROELECTRONICS INTERNATIONAL N.V. BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

/**
 * @file   vl53l1_platform.c
 * @brief  Code function definitions for Crazyflie
 *
 */
#include <stdio.h>      // sprintf(), vsnprintf(), printf()
#include <stdint.h>
#include <string.h>     // strncpy(), strnlen()

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "driver/i2c_master.h"
#include "vl53l1_core.h"
#include "vl53l1x.h"

static const char* TAG = "VL53X1_lib";

#define VL53L1_get_register_name(a,b)

static i2c_master_bus_handle_t i2c1_bus_handle = NULL;
static i2c_master_dev_handle_t vl53l1x_dev_handle = NULL;

VL53L1_Dev_t dev;

static bool s_inited = false;


// Set the start address 8 step after the VL53L0 dynamic addresses
//static int nextI2CAddress = VL53L1X_DEFAULT_ADDRESS+8;

/*
bool vl53l1xInit(VL53L1_Dev_t *pdev, I2C_Dev *I2cHandle)
{
  VL53L1_Error status = VL53L1_ERROR_NONE;

  pdev->I2cDevAddr = VL53L1X_DEFAULT_ADDRESS;
  pdev->I2Cx = I2cHandle;
  i2cdevInit(pdev->I2Cx);

  // Move initialized sensor to a new I2C address
  //int newAddress;

  //taskENTER_CRITICAL();
  //newAddress = nextI2CAddress++;
  //taskEXIT_CRITICAL();

  //vl53l1xSetI2CAddress(pdev, newAddress);

  uint8_t byteData;
  uint16_t wordData;
  VL53L1_RdByte(pdev, 0x010F, &byteData);
  ESP_LOGI(TAG,"VL53L1X Model_ID: %02X", byteData);
  VL53L1_RdByte(pdev, 0x0110, &byteData);
  ESP_LOGI(TAG,"VL53L1X Module_Type: %02X", byteData);
  VL53L1_RdWord(pdev, 0x010F, &wordData);
  ESP_LOGI(TAG,"VL53L1X: %02X", wordData);

  status = VL53L1_WaitDeviceBooted(pdev);
  if (status == VL53L1_ERROR_NONE)
  {
	status = VL53L1_DataInit(pdev);

	if (status == VL53L1_ERROR_NONE)
	{
		status = VL53L1_StaticInit(pdev);
	}
  }

#ifdef SET_VL53LX_ROI
  VL53L1_UserRoi_t Roi0;
  Roi0.TopLeftX = 0; //set ROI according to requirement
  Roi0.TopLeftY = 15;
  Roi0.BotRightX = 15;
  Roi0.BotRightY = 0;
  status = VL53L1_SetUserROI(pdev, &Roi0); //SET region of interest
#endif
	//Restart sensor remove to zranger task
	//VL53L1_StopMeasurement(pdev);
	//status = VL53L1_SetDistanceMode(pdev,VL53L1_DISTANCEMODE_LONG);
	//status = VL53L1_SetMeasurementTimingBudgetMicroSeconds(pdev, 160000);
	//status = VL53L1_SetInterMeasurementPeriodMilliSeconds(pdev, 200);  
	//status = VL53L1_StartMeasurement(pdev);

  return status == VL53L1_ERROR_NONE;
}
*/
void vl53l1x_I2CInit(void)
{
	ESP_LOGI(TAG, "Initializing I2C1 master bus...");
	// 如果还没有，则创建：
    i2c_master_bus_config_t bus_config = {
        .i2c_port = VL53L1X_I2C_NUM,
        .sda_io_num = VL53L1X_I2C_SDA_IO,
        .scl_io_num = VL53L1X_I2C_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c1_bus_handle));

    // 2. 添加 VL53L1X 设备
    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = VL53L1X_DEFAULT_ADDR,
        .scl_speed_hz = VL53L1X_I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c1_bus_handle, &dev_config, &vl53l1x_dev_handle));

    // 3. 初始化传感器结构体
    dev.i2c_dev = vl53l1x_dev_handle;
    dev.I2cDevAddr = VL53L1X_DEFAULT_ADDR;
	ESP_LOGI(TAG, "I2C1 master bus initialized (address 0x%02X, freq %d Hz)", VL53L1X_DEFAULT_ADDR, VL53L1X_I2C_FREQ_HZ);
}


bool vl53l1xInit(void) 
{
	if (s_inited) 
	{
        return true;
    }
    VL53L1_Error status = VL53L1_WaitDeviceBooted(&dev);
    if (status != VL53L1_ERROR_NONE) 
	{
        ESP_LOGE(TAG, "VL53L1_WaitDeviceBooted failed: %d", status);
        return false;
    }

    status = VL53L1_DataInit(&dev);
    if (status != VL53L1_ERROR_NONE) 
	{
        ESP_LOGE(TAG, "VL53L1_DataInit failed: %d", status);
        return false;
    }
	status = VL53L1_StaticInit(&dev);
    if (status != VL53L1_ERROR_NONE) 
	{
        ESP_LOGE(TAG, "VL53L1_StaticInit failed: %d", status);
        return false;
    }
	// 设置测距模式
	status = VL53L1_SetPresetMode(&dev, VL53L1_PRESETMODE_LITE_RANGING);
    if (status != VL53L1_ERROR_NONE) 
	{
        ESP_LOGE(TAG, "VL53L1_SetPresetMode failed: %d", status);
        return false;
    }
	// 设置长距离模式
    status = VL53L1_SetDistanceMode(&dev, VL53L1_DISTANCEMODE_LONG);
    if (status != VL53L1_ERROR_NONE) 
	{
        ESP_LOGE(TAG, "VL53L1_SetDistanceMode failed: %d", status);
        return false;
    }
	// 设置时序预算，例如 150ms（150000us）
    status = VL53L1_SetMeasurementTimingBudgetMicroSeconds(&dev, 16000);
    if (status != VL53L1_ERROR_NONE) 
	{
        ESP_LOGE(TAG, "VL53L1_SetMeasurementTimingBudget failed: %d", status);
        return false;
    }
	// 确保当前不在测量状态
    VL53L1_State state;
    if (VL53L1_GetPalState(&dev, &state) == VL53L1_ERROR_NONE && state == VL53L1_STATE_RUNNING) 
	{
        VL53L1_StopMeasurement(&dev);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

	s_inited = true;
    ESP_LOGI(TAG, "VL53L1X init ok");
	return true;
}

bool vl53l1x_single_ranging(int16_t *distance_mm, uint8_t *range_status, uint32_t timeout_ms) 
{
    VL53L1_Error err;
    VL53L1_RangingMeasurementData_t data;
    uint8_t ready = 0;
    VL53L1_State state;
    TickType_t start;

    if (!s_inited) return false;

    // ---------- 0. 确保 IDLE + 清理残局 ----------
    if (VL53L1_GetPalState(&dev, &state) == VL53L1_ERROR_NONE &&
        state == VL53L1_STATE_RUNNING) {
        VL53L1_StopMeasurement(&dev);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    VL53L1_clear_interrupt(&dev);
    vTaskDelay(pdMS_TO_TICKS(5));

    // ---------- 1. Start ----------
    err = VL53L1_StartMeasurement(&dev);
    if (err != VL53L1_ERROR_NONE) {
        ESP_LOGE(TAG, "Start failed: %d", err);
        return false;
    }

    // 关键：给传感器启动时间。绝不立刻轮询。
    vTaskDelay(pdMS_TO_TICKS(20));

    // ---------- 2. 等 sync（第一次 ready，丢弃） ----------
    ready = 0;
    start = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(timeout_ms)) {
        err = VL53L1_GetMeasurementDataReady(&dev, &ready);
        if (err == VL53L1_ERROR_NONE && ready) break;
        // 单次 I2C 错误不立刻放弃，继续轮询
        vTaskDelay(pdMS_TO_TICKS(20));   // 不要小于 20ms
    }
    if (!ready) {
        ESP_LOGW(TAG, "sync timeout");
        VL53L1_StopMeasurement(&dev);
        vTaskDelay(pdMS_TO_TICKS(20));
        return false;
    }

    VL53L1_GetRangingMeasurementData(&dev, &data);   // 读走 sync
    vTaskDelay(pdMS_TO_TICKS(5));

    // ---------- 3. 清中断 + 启下一轮 ----------
    err = VL53L1_ClearInterruptAndStartMeasurement(&dev);
    if (err != VL53L1_ERROR_NONE) {
        ESP_LOGE(TAG, "ClearInterrupt failed: %d", err);
        VL53L1_StopMeasurement(&dev);
        vTaskDelay(pdMS_TO_TICKS(50));
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(20));   // 同样给它启动时间

    // ---------- 4. 等 real（第二次 ready，真数据） ----------
    ready = 0;
    start = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(timeout_ms)) {
        err = VL53L1_GetMeasurementDataReady(&dev, &ready);
        if (err == VL53L1_ERROR_NONE && ready) break;
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    if (!ready) {
        ESP_LOGW(TAG, "real timeout");
        VL53L1_StopMeasurement(&dev);
        vTaskDelay(pdMS_TO_TICKS(50));
        return false;
    }

    err = VL53L1_GetRangingMeasurementData(&dev, &data);
    if (err != VL53L1_ERROR_NONE) {
        VL53L1_StopMeasurement(&dev);
        vTaskDelay(pdMS_TO_TICKS(50));
        return false;
    }

    // ---------- 5. Stop，等传感器真正回 standby ----------
    VL53L1_StopMeasurement(&dev);
    vTaskDelay(pdMS_TO_TICKS(50));   // 不要少于 50ms

    if (distance_mm) *distance_mm = data.RangeMilliMeter;
    if (range_status) *range_status = data.RangeStatus;
    return true;
}


bool vl53l1xTestConnection(VL53L1_Dev_t* pdev)
{
	VL53L1_DeviceInfo_t info;
	VL53L1_Error status = VL53L1_ERROR_NONE;

	status = VL53L1_GetDeviceInfo(pdev, &info);

	return status == VL53L1_ERROR_NONE;
}

/** Set I2C address
 * Any subsequent communication will be on the new address
 * The address passed is the 7bit I2C address from LSB (ie. without the
 * read/write bit)
 */
VL53L1_Error vl53l1xSetI2CAddress(VL53L1_Dev_t* pdev, uint8_t address)
{
	VL53L1_Error status = VL53L1_ERROR_NONE;

	status = VL53L1_SetDeviceAddress(pdev, address);
	pdev->I2cDevAddr = address;
	return  status;
}


/*
 * ----------------- COMMS FUNCTIONS -----------------
 */
/*
VL53L1_Error VL53L1_WriteMulti(
	VL53L1_Dev_t *pdev,
	uint16_t      index,
	uint8_t      *pdata,
	uint32_t      count)
{
	VL53L1_Error status         = VL53L1_ERROR_NONE;

  if (!i2cdevWrite16(pdev->I2Cx, pdev->I2cDevAddr, index, count, pdata))
  {
    status = VL53L1_ERROR_CONTROL_INTERFACE;
  }

	return status;
}*/
VL53L1_Error VL53L1_WriteMulti(VL53L1_Dev_t *pdev, uint16_t index, uint8_t *pdata, uint32_t count) {
    uint8_t buffer[2 + count];
    buffer[0] = (index >> 8) & 0xFF;
    buffer[1] = index & 0xFF;
    if (pdata && count) memcpy(buffer + 2, pdata, count);
    esp_err_t err = i2c_master_transmit(pdev->i2c_dev, buffer, 2 + count, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "WriteMulti error: %d", err);
        return VL53L1_ERROR_CONTROL_INTERFACE;
    }
    return VL53L1_ERROR_NONE;
}
/*
VL53L1_Error VL53L1_ReadMulti(
	VL53L1_Dev_t *pdev,
	uint16_t      index,
	uint8_t      *pdata,
	uint32_t      count)
{
	VL53L1_Error status         = VL53L1_ERROR_NONE;

  if (!i2cdevRead16(pdev->I2Cx, pdev->I2cDevAddr, index, count, pdata))
  {
    status = VL53L1_ERROR_CONTROL_INTERFACE;
  }

	return status;
}*/
VL53L1_Error VL53L1_ReadMulti(VL53L1_Dev_t *pdev, uint16_t index, uint8_t *pdata, uint32_t count) {
    uint8_t reg_buf[2] = { (index >> 8) & 0xFF, index & 0xFF };
    esp_err_t err = i2c_master_transmit_receive(pdev->i2c_dev, reg_buf, 2, pdata, count, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ReadMulti error: %d", err);
        return VL53L1_ERROR_CONTROL_INTERFACE;
    }
    return VL53L1_ERROR_NONE;
}

/*
VL53L1_Error VL53L1_WrByte(
	VL53L1_Dev_t *pdev,
	uint16_t      index,
	uint8_t       data)
{
	VL53L1_Error status         = VL53L1_ERROR_NONE;

	if (!i2cdevWrite16(pdev->I2Cx, pdev->I2cDevAddr, index, 1, &data))
	{
	  status = VL53L1_ERROR_CONTROL_INTERFACE;
	}

	return status;
}*/
VL53L1_Error VL53L1_WrByte(VL53L1_Dev_t *pdev, uint16_t index, uint8_t data) {
    return VL53L1_WriteMulti(pdev, index, &data, 1);
}
/*
VL53L1_Error VL53L1_WrWord(
	VL53L1_Dev_t *pdev,
	uint16_t      index,
	uint16_t      data)
{
  VL53L1_Error status         = VL53L1_ERROR_NONE;
  uint8_t _I2CBuffer[2];
  _I2CBuffer[0] = data >> 8;
  _I2CBuffer[1] = data & 0x00FF;
  if (!i2cdevWrite16(pdev->I2Cx, pdev->I2cDevAddr, index, 2, (uint8_t *)_I2CBuffer))
  {
    status = VL53L1_ERROR_CONTROL_INTERFACE;
  }

	return status;
}*/
VL53L1_Error VL53L1_WrWord(VL53L1_Dev_t *pdev, uint16_t index, uint16_t data) {
    uint8_t buf[2] = { (data >> 8) & 0xFF, data & 0xFF };
    return VL53L1_WriteMulti(pdev, index, buf, 2);
}

/*
VL53L1_Error VL53L1_WrDWord(
	VL53L1_Dev_t *pdev,
	uint16_t      index,
	uint32_t      data)
{
	VL53L1_Error status = VL53L1_ERROR_NONE;
  	uint8_t _I2CBuffer[4];
	_I2CBuffer[0] = (data >> 24) & 0xFF;
	_I2CBuffer[1] = (data >> 16) & 0xFF;
	_I2CBuffer[2] = (data >> 8) & 0xFF;
	_I2CBuffer[3] = (data >> 0) & 0xFF;

	if (!i2cdevWrite16(pdev->I2Cx, pdev->I2cDevAddr, index, 4, (uint8_t *)_I2CBuffer))
  {
    status = VL53L1_ERROR_CONTROL_INTERFACE;
  }

	return status;
}*/
VL53L1_Error VL53L1_WrDWord(VL53L1_Dev_t *pdev, uint16_t index, uint32_t data) {
    uint8_t buf[4] = { (data >> 24) & 0xFF, (data >> 16) & 0xFF, (data >> 8) & 0xFF, data & 0xFF };
    return VL53L1_WriteMulti(pdev, index, buf, 4);
}
/*
VL53L1_Error VL53L1_RdByte(
	VL53L1_Dev_t *pdev,
	uint16_t      index,
	uint8_t      *pdata)
{
	VL53L1_Error status         = VL53L1_ERROR_NONE;

	if (!i2cdevRead16(pdev->I2Cx, pdev->I2cDevAddr, index, 1, pdata))
  {
    status = VL53L1_ERROR_CONTROL_INTERFACE;
  }

	return status;
}*/
VL53L1_Error VL53L1_RdByte(VL53L1_Dev_t *pdev, uint16_t index, uint8_t *pdata) {
    return VL53L1_ReadMulti(pdev, index, pdata, 1);
}
/*
VL53L1_Error VL53L1_RdWord(
	VL53L1_Dev_t *pdev,
	uint16_t index,
	uint16_t *pdata)
{
	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t _I2CBuffer[2];

	if (!i2cdevRead16(pdev->I2Cx, pdev->I2cDevAddr, index, 2, (uint8_t *)_I2CBuffer))
	{
		status = VL53L1_ERROR_CONTROL_INTERFACE;
	}
	*pdata = ((uint16_t)_I2CBuffer[0] << 8) + (uint16_t)_I2CBuffer[1];
	return status;
}*/
VL53L1_Error VL53L1_RdWord(VL53L1_Dev_t *pdev, uint16_t index, uint16_t *pdata) {
    uint8_t buf[2];
    VL53L1_Error err = VL53L1_ReadMulti(pdev, index, buf, 2);
    if (err == VL53L1_ERROR_NONE) *pdata = (buf[0] << 8) | buf[1];
    return err;
}
/*
VL53L1_Error VL53L1_RdDWord(
	VL53L1_Dev_t *pdev,
	uint16_t index,
	uint32_t *pdata)
{
	VL53L1_Error status = VL53L1_ERROR_NONE;
	uint8_t _I2CBuffer[4];
	if (!i2cdevRead16(pdev->I2Cx, pdev->I2cDevAddr, index, 4, (uint8_t *)_I2CBuffer))
	{
		status = VL53L1_ERROR_CONTROL_INTERFACE;
	}
	*pdata = ((uint32_t)_I2CBuffer[0] << 24) + ((uint32_t)_I2CBuffer[1] << 16) + ((uint32_t)_I2CBuffer[2] << 8) + (uint32_t)_I2CBuffer[3];
	return status;
}*/
VL53L1_Error VL53L1_RdDWord(VL53L1_Dev_t *pdev, uint16_t index, uint32_t *pdata) {
    uint8_t buf[4];
    VL53L1_Error err = VL53L1_ReadMulti(pdev, index, buf, 4);
    if (err == VL53L1_ERROR_NONE) *pdata = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    return err;
}

/*
 * ----------------- HOST TIMING FUNCTIONS -----------------
 */
/*
VL53L1_Error VL53L1_WaitUs(
	VL53L1_Dev_t *pdev,
	int32_t       wait_us)
{
	VL53L1_Error status         = VL53L1_ERROR_NONE;
	uint32_t delay_ms = (wait_us + 900) / 1000;

	if(delay_ms == 0)
	{
	  delay_ms = 1;
	}

	vTaskDelay(pdMS_TO_TICKS(delay_ms));

	return status;
}*/
VL53L1_Error VL53L1_WaitUs(VL53L1_Dev_t *pdev, int32_t wait_us) {
    uint32_t delay_ms = (wait_us + 900) / 1000;
    if (delay_ms == 0) delay_ms = 1;
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
    return VL53L1_ERROR_NONE;
}
/*
VL53L1_Error VL53L1_WaitMs(
	VL53L1_Dev_t *pdev,
	int32_t       wait_ms)
{
  vTaskDelay(pdMS_TO_TICKS(wait_ms));

  return VL53L1_ERROR_NONE;
}*/
VL53L1_Error VL53L1_WaitMs(VL53L1_Dev_t *pdev, int32_t wait_ms) {
    vTaskDelay(pdMS_TO_TICKS(wait_ms));
    return VL53L1_ERROR_NONE;
}

/*
 * ----------------- DEVICE TIMING FUNCTIONS -----------------
 */

/*
VL53L1_Error VL53L1_GetTickCount(
	uint32_t *ptick_count_ms)
{
	///////// Returns current tick count in [ms] 
	*ptick_count_ms = xTaskGetTickCount();

	return VL53L1_ERROR_NONE;
}*/
VL53L1_Error VL53L1_GetTickCount(uint32_t *ptick_count_ms) {
    *ptick_count_ms = xTaskGetTickCount();
    return VL53L1_ERROR_NONE;
}
/*
VL53L1_Error VL53L1_WaitValueMaskEx(
	VL53L1_Dev_t *pdev,
	uint32_t      timeout_ms,
	uint16_t      index,
	uint8_t       value,
	uint8_t       mask,
	uint32_t      poll_delay_ms)
{
	//
	 // Platform implementation of WaitValueMaskEx V2WReg script command
	 //
	 // WaitValueMaskEx(
	 //          duration_ms,
	 //          index,
	 //          value,
	 //          mask,
	 //          poll_delay_ms);
	 //

	VL53L1_Error status         = VL53L1_ERROR_NONE;
	uint32_t     start_time_ms   = 0;
	uint32_t     current_time_ms = 0;
	uint8_t      byte_value      = 0;
	uint8_t      found           = 0;
#ifdef VL53L1_LOG_ENABLE
	uint32_t     trace_functions = 0;
#endif

	SUPPRESS_UNUSED_WARNING(poll_delay_ms);

#ifdef VL53L1_LOG_ENABLE
	// look up register name
	VL53L1_get_register_name(
			index,
			register_name);

	// Output to I2C logger for FMT/DFT
	trace_i2c("WaitValueMaskEx(%5d, %s, 0x%02X, 0x%02X, %5d);\n",
		timeout_ms, register_name, value, mask, poll_delay_ms);
#endif // VL53L1_LOG_ENABLE

	// calculate time limit in absolute time

	VL53L1_GetTickCount(&start_time_ms);
	pdev->new_data_ready_poll_duration_ms = 0;

	// remember current trace functions and temporarily disable
	// function logging


#ifdef VL53L1_LOG_ENABLE
	trace_functions = _LOG_GET_TRACE_FUNCTIONS();
#endif

	// wait until value is found, timeout reached on error occurred 

	while ((status == VL53L1_ERROR_NONE) &&
		   (pdev->new_data_ready_poll_duration_ms < timeout_ms) &&
		   (found == 0))
	{
		status = VL53L1_RdByte(
						pdev,
						index,
						&byte_value);

		if ((byte_value & mask) == value)
		{
			found = 1;
		}

		if (status == VL53L1_ERROR_NONE  &&
			found == 0 &&
			poll_delay_ms > 0)
			status = VL53L1_WaitMs(
							pdev,
							poll_delay_ms);

		// Update polling time (Compare difference rather than absolute to negate 32bit wrap around issue)
		VL53L1_GetTickCount(&current_time_ms);
		pdev->new_data_ready_poll_duration_ms = current_time_ms - start_time_ms;
	}

	if (found == 0 && status == VL53L1_ERROR_NONE)
		status = VL53L1_ERROR_TIME_OUT;

	return status;
}*/
VL53L1_Error VL53L1_WaitValueMaskEx(VL53L1_Dev_t *pdev, uint32_t timeout_ms, uint16_t index,
                                    uint8_t value, uint8_t mask, uint32_t poll_delay_ms) {
    uint32_t start = xTaskGetTickCount();
    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(timeout_ms)) {
        uint8_t byte_value;
        VL53L1_Error err = VL53L1_RdByte(pdev, index, &byte_value);
        if (err != VL53L1_ERROR_NONE) return err;
        if ((byte_value & mask) == value) return VL53L1_ERROR_NONE;
        if (poll_delay_ms > 0) vTaskDelay(pdMS_TO_TICKS(poll_delay_ms));
    }
    return VL53L1_ERROR_TIME_OUT;
}



void ranging_test(void)
{
	uint8_t data_ready = 0;
    VL53L1_GetMeasurementDataReady(&dev, &data_ready);
    if (data_ready) 
	{
        VL53L1_RangingMeasurementData_t data;
        VL53L1_GetRangingMeasurementData(&dev, &data);
        VL53L1_ClearInterruptAndStartMeasurement(&dev);
        if (data.RangeStatus == 0) 
		{
            ESP_LOGI("APP", "Distance: %d mm", data.RangeMilliMeter);
        } 
		else 
		{
            ESP_LOGW("APP", "Invalid range, status: %d", data.RangeStatus);
        }
    }
}