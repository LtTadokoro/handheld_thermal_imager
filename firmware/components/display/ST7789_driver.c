#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_commands.h>
#include <driver/spi_master.h>
#include <esp_log.h>
#include "ST7789_driver.h"
#include "config.h"

#define LCD_SPI_HOST    SPI2_HOST

static const char* TAG = "st7789";

// 创建 ST7789 面板实例
esp_lcd_panel_handle_t panel_handle = NULL;

//lcd操作句柄
static esp_lcd_panel_io_handle_t lcd_io_handle = NULL;

//刷新完成回调函数
static lcd_flush_done_cb    s_flush_done_cb = NULL;

//背光GPIO
static gpio_num_t   s_bl_gpio = -1;

static bool notify_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx)
{
    if(s_flush_done_cb)
        s_flush_done_cb(user_ctx);
    return false;
}


void lcd_init(void)
{
    st7789_cfg_t st7789_config;
    st7789_config.mosi = LCD_MOSI_PIN;
    st7789_config.clk = LCD_SCL_PIN;
    st7789_config.cs = LCD_CS_PIN;
    st7789_config.dc = LCD_DC_PIN;
    st7789_config.rst = LCD_RES_PIN;
    st7789_config.bl = BLK_PIN;
    st7789_config.spi_fre = 80*1000*1000;       //SPI时钟频率
    st7789_config.width = LCD_WIDTH;            //屏宽
    st7789_config.height = LCD_HEIGHT;          //屏高
    st7789_config.spin = LCD_SPAN;                     //顺时针旋转90度
    st7789_config.done_cb = NULL;    //数据写入完成回调函数
    st7789_config.cb_param = NULL;         //回调函数参数

    st7789_driver_hw_init(&st7789_config);
}


/** st7789初始化
 * @param st7789_cfg_t  接口参数
 * @return 成功或失败
*/
esp_err_t st7789_driver_hw_init(st7789_cfg_t* cfg)
{
    //初始化SPI
    spi_bus_config_t buscfg = {
        .sclk_io_num = cfg->clk,        //SCLK引脚
        .mosi_io_num = cfg->mosi,       //MOSI引脚
        .miso_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .flags = SPICOMMON_BUSFLAG_MASTER , //SPI主模式
        .max_transfer_sz = cfg->width * 40 * sizeof(uint16_t),  //DMA单次传输最大字节，最大32768
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    s_flush_done_cb = cfg->done_cb; //设置刷新完成回调函数

    //初始化片选引脚
    if(cfg->cs > 0)
    {
        gpio_config_t cs_gpio_cfg = 
        {
            .pull_up_en = GPIO_PULLUP_DISABLE,          //禁止上拉
            .pull_down_en = GPIO_PULLDOWN_DISABLE,      //禁止下拉
            .mode = GPIO_MODE_OUTPUT,                   //输出模式
            .intr_type = GPIO_INTR_DISABLE,             //禁止中断
            .pin_bit_mask = (1<<cfg->cs)                //GPIO脚
        };
        gpio_config(&cs_gpio_cfg);
    }

    //初始化复位引脚
    if(cfg->rst > 0)
    {
        gpio_config_t rst_gpio_cfg = 
        {
            .pull_up_en = GPIO_PULLUP_DISABLE,          //禁止上拉
            .pull_down_en = GPIO_PULLDOWN_DISABLE,      //禁止下拉
            .mode = GPIO_MODE_OUTPUT,                   //输出模式
            .intr_type = GPIO_INTR_DISABLE,             //禁止中断
            .pin_bit_mask = (1<<cfg->rst)                //GPIO脚
        };
        gpio_config(&rst_gpio_cfg);
    }

    //初始化GPIO(背光引脚)
    if(cfg->bl >0)
    {
        gpio_config_t bl_gpio_cfg = 
        {
            .pull_up_en = GPIO_PULLUP_DISABLE,          //禁止上拉
            .pull_down_en = GPIO_PULLDOWN_DISABLE,      //禁止下拉
            .mode = GPIO_MODE_OUTPUT,                   //输出模式
            .intr_type = GPIO_INTR_DISABLE,             //禁止中断
            .pin_bit_mask = (1<<cfg->bl)                //GPIO脚
        };
        gpio_config(&bl_gpio_cfg);
        s_bl_gpio = cfg->bl;    //设置背光GPIO
    }

    //创建基于spi的lcd操作句柄
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = cfg->dc,         //DC引脚
        .cs_gpio_num = cfg->cs,         //CS引脚
        .pclk_hz = cfg->spi_fre,        //SPI时钟频率
        .lcd_cmd_bits = 8,              //命令长度
        .lcd_param_bits = 8,            //参数长度
        .spi_mode = 0,                  //使用SPI0模式
        .trans_queue_depth = 10,        //表示可以缓存的spi传输事务队列深度
        .on_color_trans_done = notify_flush_ready,   //刷新完成回调函数
        .user_ctx = cfg->cb_param,                                    //回调函数参数
        .flags = {    // 以下为 SPI 时序的相关参数，需根据 LCD 驱动 IC 的数据手册以及硬件的配置确定
            .sio_mode = 0,    // 通过一根数据线（MOSI）读写数据，0: Interface I 型，1: Interface II 型
        },
    };
    // Attach the LCD to the SPI bus
    ESP_LOGI(TAG,"create esp_lcd_new_panel");
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_SPI_HOST, &io_config, &lcd_io_handle));
    
    //硬件复位
    /*
    if(cfg->rst > 0)
    {
        gpio_set_level(cfg->rst,0);
        vTaskDelay(pdMS_TO_TICKS(20));
        gpio_set_level(cfg->rst,1);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
        */

    // 1. 定义面板配置
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = cfg->rst,
        .rgb_ele_order = LCD_RGB_ENDIAN_RGB,
        .bits_per_pixel = 16,  // 这里强制指定 16位
        .flags = {
            .reset_active_high = 0, // 复位低电平有效
        },
    };
    
    esp_lcd_new_panel_st7789(lcd_io_handle, &panel_cfg, &panel_handle);

    // 3. 由官方驱动执行标准的初始化序列（包括 0x3A 0x55）
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);

    // 4. 设置旋转
    esp_lcd_panel_swap_xy(panel_handle,cfg->spin);

    // 5. 设置颜色反转(for ips screen)
    #ifdef IPS_screen
    esp_lcd_panel_invert_color(panel_handle, true);
    #endif
    #ifdef TFT_screen
    esp_lcd_panel_invert_color(panel_handle, false);
    #endif

    // 6. 暂时关闭显示
    esp_lcd_panel_disp_on_off(panel_handle, false);

    return ESP_OK;
}

/** st7789写入显示数据
 * @param x1,x2,y1,y2:显示区域
 * @return 无
*/
void st7789_flush(int x1,int x2,int y1,int y2,void *data)
{
    if(x2 <= x1 || y2 <= y1)
    {
        if(s_flush_done_cb)
            s_flush_done_cb(NULL);
        return;
    }
    esp_lcd_panel_io_tx_param(lcd_io_handle, LCD_CMD_CASET, (uint8_t[]) {
        (x1 >> 8) & 0xFF,
        x1 & 0xFF,
        ((x2 - 1) >> 8) & 0xFF,
        (x2 - 1) & 0xFF,
    }, 4);
    esp_lcd_panel_io_tx_param(lcd_io_handle, LCD_CMD_RASET, (uint8_t[]) {
        (y1 >> 8) & 0xFF,
        y1 & 0xFF,
        ((y2 - 1) >> 8) & 0xFF,
        (y2 - 1) & 0xFF,
    }, 4);
    // transfer frame buffer
    size_t len = (x2 - x1) * (y2 - y1) * 2;
    esp_lcd_panel_io_tx_color(lcd_io_handle, LCD_CMD_RAMWR, data, len);
    if (s_flush_done_cb) s_flush_done_cb(NULL);
    return;
}

/** 控制背光
 * @param enable 是否使能背光
 * @return 无
*/
void st7789_lcd_backlight(bool enable)
{
    if(enable)
    {
        gpio_set_level(s_bl_gpio,1);
    }
    else
    {
        gpio_set_level(s_bl_gpio,0);
    }
}

void turn_on_display(void)
{
    esp_lcd_panel_disp_on_off(panel_handle, true);
}