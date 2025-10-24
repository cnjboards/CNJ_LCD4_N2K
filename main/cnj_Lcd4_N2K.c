/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <lvgl.h>
#include <string.h>
#include <float.h>
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_mac.h"
#include <esp_http_server.h>
#include <esp_ota_ops.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <esp_wifi.h>
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_io_additions.h"
#include "esp_lcd_touch_gt911.h"
#include "esp_lcd_st7701.h"
#include "driver/ledc.h"
#include "lv_demos.h"
#include "lvgl_port.h"
#include "esp_io_expander_tca9554.h"
#include "driver/gpio.h"
#include "sw6106.h"
#include "OwnN2K.h"
#include "waveshare-display.h"
#include "wifi.h"

// used to boot the board without a screen
#define INCLUDE_DISPLAY

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define EXAMPLE_LCD_H_RES (480)
#define EXAMPLE_LCD_V_RES (480)
#define EXAMPLE_LCD_BIT_PER_PIXEL (18)
#define EXAMPLE_RGB_BIT_PER_PIXEL (16)
#define EXAMPLE_RGB_DATA_WIDTH (16)
#define EXAMPLE_RGB_BOUNCE_BUFFER_SIZE (EXAMPLE_LCD_H_RES * CONFIG_EXAMPLE_LCD_RGB_BOUNCE_BUFFER_HEIGHT)
#define EXAMPLE_LCD_IO_RGB_DISP (-1) // -1 if not used
#define EXAMPLE_LCD_IO_RGB_VSYNC (GPIO_NUM_39)
#define EXAMPLE_LCD_IO_RGB_HSYNC (GPIO_NUM_38)
#define EXAMPLE_LCD_IO_RGB_DE (GPIO_NUM_40)
#define EXAMPLE_LCD_IO_RGB_PCLK (GPIO_NUM_41)
#define EXAMPLE_LCD_IO_RGB_DATA0 (GPIO_NUM_5)
#define EXAMPLE_LCD_IO_RGB_DATA1 (GPIO_NUM_45)
#define EXAMPLE_LCD_IO_RGB_DATA2 (GPIO_NUM_48)
#define EXAMPLE_LCD_IO_RGB_DATA3 (GPIO_NUM_47)
#define EXAMPLE_LCD_IO_RGB_DATA4 (GPIO_NUM_21)
#define EXAMPLE_LCD_IO_RGB_DATA5 (GPIO_NUM_14)
#define EXAMPLE_LCD_IO_RGB_DATA6 (GPIO_NUM_13)
#define EXAMPLE_LCD_IO_RGB_DATA7 (GPIO_NUM_12)
#define EXAMPLE_LCD_IO_RGB_DATA8 (GPIO_NUM_11)
#define EXAMPLE_LCD_IO_RGB_DATA9 (GPIO_NUM_10)
#define EXAMPLE_LCD_IO_RGB_DATA10 (GPIO_NUM_9)
#define EXAMPLE_LCD_IO_RGB_DATA11 (GPIO_NUM_46)
#define EXAMPLE_LCD_IO_RGB_DATA12 (GPIO_NUM_3)
#define EXAMPLE_LCD_IO_RGB_DATA13 (GPIO_NUM_8)
#define EXAMPLE_LCD_IO_RGB_DATA14 (GPIO_NUM_18)
#define EXAMPLE_LCD_IO_RGB_DATA15 (GPIO_NUM_17)
#define EXAMPLE_LCD_IO_SPI_CS (GPIO_NUM_42)
#define EXAMPLE_LCD_IO_SPI_SCL (GPIO_NUM_2)
#define EXAMPLE_LCD_IO_SPI_SDA (GPIO_NUM_1)
#define EXAMPLE_LCD_IO_RST (-1)       // -1 if not used
#define EXAMPLE_PIN_NUM_BK_LIGHT (/*GPIO_NUM_4*/-1) // -1 if not used
#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL (1)
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your touch spec ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#if CONFIG_EXAMPLE_LCD_TOUCH_CONTROLLER_GT911
#define TOUCH_HOST (I2C_NUM_0)
#define EXAMPLE_PIN_NUM_TOUCH_SCL (GPIO_NUM_7)
#define EXAMPLE_PIN_NUM_TOUCH_SDA (GPIO_NUM_15)
#define EXAMPLE_PIN_NUM_TOUCH_RST (-1)          // -1 if not used
#define EXAMPLE_PIN_NUM_TOUCH_INT (GPIO_NUM_16) // -1 if not used
//#endif

// forward declaration
static void ledc_init(void);

extern int OneWireInit();

#define NODE "CNJ_LCD4_N2K"
static const char *TAG = NODE;
#define WIFI_SSID NODE /*"ESP32 OTA Update"*/
// chipid of the host
uint32_t chipId;
// global flag for startup complete
bool startUpDelayDone=false;

esp_io_expander_handle_t io_expander = NULL;
esp_io_expander_handle_t io_expander_ext = NULL;
esp_lcd_panel_handle_t lcd_handle = NULL;

IRAM_ATTR static bool rgb_lcd_on_vsync_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *edata, void *user_ctx)
{
    return lvgl_port_notify_rgb_vsync();
}

#ifdef INCLUDE_DISPLAY
static const st7701_lcd_init_cmd_t lcd_init_cmds[] = {
    //  {cmd, { data }, data_size, delay_ms}
    {0x11, (uint8_t[]){0x00}, 0, 120},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x10}, 5, 0},
    {0xC0, (uint8_t[]){0x3B, 0x00}, 2, 0},
    {0xC1, (uint8_t[]){0x0D, 0x02}, 2, 0},
    {0xC2, (uint8_t[]){0x21, 0x08}, 2, 0},
    {0xCD, (uint8_t[]){0x08}, 1, 0},
    {0xB0, (uint8_t[]){0x00, 0x11, 0x18, 0x0E, 0x11, 0x06, 0x07, 0x08, 0x07, 0x22, 0x04, 0x12, 0x0F, 0xAA, 0x31, 0x18}, 16, 0},
    {0xB1, (uint8_t[]){0x00, 0x11, 0x19, 0x0E, 0x12, 0x07, 0x08, 0x08, 0x08, 0x22, 0x04, 0x11, 0x11, 0xA9, 0x32, 0x18}, 16, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x11}, 5, 0},
    {0xB0, (uint8_t[]){0x60}, 1, 0},
    {0xB1, (uint8_t[]){0x30}, 1, 0},
    {0xB2, (uint8_t[]){0x87}, 1, 0},
    {0xB3, (uint8_t[]){0x80}, 1, 0},
    {0xB5, (uint8_t[]){0x49}, 1, 0},
    {0xB7, (uint8_t[]){0x85}, 1, 0},
    {0xB8, (uint8_t[]){0x21}, 1, 0},
    {0xC1, (uint8_t[]){0x78}, 1, 0},
    {0xC2, (uint8_t[]){0x78}, 1, 20},
    {0xE0, (uint8_t[]){0x00, 0x1B, 0x02}, 3, 0},
    {0xE1, (uint8_t[]){0x08, 0xA0, 0x00, 0x00, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x44, 0x44}, 11, 0},
    {0xE2, (uint8_t[]){0x11, 0x11, 0x44, 0x44, 0xED, 0xA0, 0x00, 0x00, 0xEC, 0xA0, 0x00, 0x00}, 12, 0},
    {0xE3, (uint8_t[]){0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0xE4, (uint8_t[]){0x44, 0x44}, 2, 0},
    {0xE5, (uint8_t[]){0x0A, 0xE9, 0xD8, 0xA0, 0x0C, 0xEB, 0xD8, 0xA0, 0x0E, 0xED, 0xD8, 0xA0, 0x10, 0xEF, 0xD8, 0xA0}, 16, 0},
    {0xE6, (uint8_t[]){0x00, 0x00, 0x11, 0x11}, 4, 0},
    {0xE7, (uint8_t[]){0x44, 0x44}, 2, 0},
    {0xE8, (uint8_t[]){0x09, 0xE8, 0xD8, 0xA0, 0x0B, 0xEA, 0xD8, 0xA0, 0x0D, 0xEC, 0xD8, 0xA0, 0x0F, 0xEE, 0xD8, 0xA0}, 16, 0},
    {0xEB, (uint8_t[]){0x02, 0x00, 0xE4, 0xE4, 0x88, 0x00, 0x40}, 7, 0},
    {0xEC, (uint8_t[]){0x3C, 0x00}, 2, 0},
    {0xED, (uint8_t[]){0xAB, 0x89, 0x76, 0x54, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x20, 0x45, 0x67, 0x98, 0xBA}, 16, 0},
    {0xFF, (uint8_t[]){0x77, 0x01, 0x00, 0x00, 0x00}, 5, 0},
    {0x36, (uint8_t[]){0x00}, 1, 0},
    {0x3A, (uint8_t[]){0x66}, 1, 0},
    {0x21, (uint8_t[]){0x00}, 0, 120},
    {0x29, (uint8_t[]){0x00}, 0, 0},
};
#endif

// set everything up  
void app_main(void)
{
    uint8_t chipid [ 6 ];
    // derive a unique chip id from the burned in MAC address
    esp_efuse_mac_get_default ( chipid );
    for ( int i = 0 ; i < 6 ; i++ )
    chipId += ( chipid [ i ] << ( 7 * i ));
    ESP_LOGI(TAG, "CHIP: %lx", chipId);

    // ***************** Start OTA ***********************
    // check nvs for ota stuff
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	} // end if
	ESP_ERROR_CHECK(ret);

	/* Mark current app as valid */
	const esp_partition_t *partition = esp_ota_get_running_partition();
	printf("Currently running partition: %s\r\n", partition->label);

    // setup wifi ap, sta and start ota webserver
    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP_STA");
    wifi_init_ap_sta();

    // ***************** Start I2C ***********************
    // setup i2c for touch screen
    ESP_LOGI(TAG, "Initialize Touch bus");
    const i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = EXAMPLE_PIN_NUM_TOUCH_SDA,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = EXAMPLE_PIN_NUM_TOUCH_SCL,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400 * 1000,
    };
    ESP_ERROR_CHECK(i2c_param_config(TOUCH_HOST, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(TOUCH_HOST, i2c_conf.mode, 0, 0, 0));

    // setup i2c for lcd touch screen
    ESP_LOGI(TAG, "Initialize IOExpander bus");

    // config the onboard gpio expander
    ret = esp_io_expander_new_i2c_tca9554(TOUCH_HOST, ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000, &io_expander);

    // config sopme gpio on expander
    esp_io_expander_set_dir(io_expander, IO_EXPANDER_PIN_NUM_5 | IO_EXPANDER_PIN_NUM_0 | IO_EXPANDER_PIN_NUM_2 | IO_EXPANDER_PIN_NUM_4, IO_EXPANDER_OUTPUT);
    esp_rom_gpio_pad_select_gpio(EXAMPLE_PIN_NUM_TOUCH_INT);
    gpio_set_direction(EXAMPLE_PIN_NUM_TOUCH_INT, GPIO_MODE_OUTPUT);
    gpio_set_level(EXAMPLE_PIN_NUM_TOUCH_INT, 0);
    esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_5, 0 /*1*/); // this is the beep, 
    esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_0, 0); // TP Reset line Exio0
    esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_2, 0); // BL Exio
    
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_5, 0);
    esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_0, 1); 
    esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_2, 1); // BL ??

    // setup i2c for lcd touch screen
    ESP_LOGI(TAG, "Initialize IOExpander bus external");

    // config the external gpio expander - test code
    esp_io_expander_new_i2c_tca9554(TOUCH_HOST, ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_001, &io_expander_ext);

    // led backlight, pwm output
    ledc_init();
    
    ESP_LOGI(TAG, "Install 3-wire SPI panel IO");
    spi_line_config_t line_config = {
        .cs_io_type = IO_TYPE_GPIO,
        .cs_gpio_num = EXAMPLE_LCD_IO_SPI_CS,
        .scl_io_type = IO_TYPE_GPIO,
        .scl_gpio_num = EXAMPLE_LCD_IO_SPI_SCL,
        .sda_io_type = IO_TYPE_GPIO,
        .sda_gpio_num = EXAMPLE_LCD_IO_SPI_SDA,
        .io_expander = NULL,
    };
    esp_lcd_panel_io_3wire_spi_config_t io_config = ST7701_PANEL_IO_3WIRE_SPI_CONFIG(line_config, 0);
    esp_lcd_panel_io_handle_t io_handle = NULL;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_3wire_spi(&io_config, &io_handle));

#ifdef INCLUDE_DISPLAY

    ESP_LOGI(TAG, "Install ST7701 panel driver");
    esp_lcd_rgb_panel_config_t rgb_config = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .psram_trans_align = 64,
        .data_width = EXAMPLE_RGB_DATA_WIDTH,
        .bits_per_pixel = EXAMPLE_RGB_BIT_PER_PIXEL,
        .de_gpio_num = EXAMPLE_LCD_IO_RGB_DE,
        .pclk_gpio_num = EXAMPLE_LCD_IO_RGB_PCLK,
        .vsync_gpio_num = EXAMPLE_LCD_IO_RGB_VSYNC,
        .hsync_gpio_num = EXAMPLE_LCD_IO_RGB_HSYNC,
        .disp_gpio_num = EXAMPLE_LCD_IO_RGB_DISP,
        .data_gpio_nums = {
            EXAMPLE_LCD_IO_RGB_DATA0,
            EXAMPLE_LCD_IO_RGB_DATA1,
            EXAMPLE_LCD_IO_RGB_DATA2,
            EXAMPLE_LCD_IO_RGB_DATA3,
            EXAMPLE_LCD_IO_RGB_DATA4,
            EXAMPLE_LCD_IO_RGB_DATA5,
            EXAMPLE_LCD_IO_RGB_DATA6,
            EXAMPLE_LCD_IO_RGB_DATA7,
            EXAMPLE_LCD_IO_RGB_DATA8,
            EXAMPLE_LCD_IO_RGB_DATA9,
            EXAMPLE_LCD_IO_RGB_DATA10,
            EXAMPLE_LCD_IO_RGB_DATA11,
            EXAMPLE_LCD_IO_RGB_DATA12,
            EXAMPLE_LCD_IO_RGB_DATA13,
            EXAMPLE_LCD_IO_RGB_DATA14,
            EXAMPLE_LCD_IO_RGB_DATA15,
        },
        .timings = ST7701_480_480_PANEL_60HZ_RGB_TIMING(),
        .flags.fb_in_psram = 1,
        .num_fbs = LVGL_PORT_LCD_RGB_BUFFER_NUMS,
        .bounce_buffer_size_px = EXAMPLE_RGB_BOUNCE_BUFFER_SIZE,
    };
    rgb_config.timings.h_res = EXAMPLE_LCD_H_RES;
    rgb_config.timings.v_res = EXAMPLE_LCD_V_RES;
    st7701_vendor_config_t vendor_config = {
        .rgb_config = &rgb_config,
        .init_cmds = lcd_init_cmds, // Uncomment these line if use custom initialization commands
        .init_cmds_size = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]),
        .flags = {
            .auto_del_panel_io = 0, /**
                                     * Set to 1 if panel IO is no longer needed after LCD initialization.
                                     * If the panel IO pins are sharing other pins of the RGB interface to save GPIOs,
                                     * Please set it to 1 to release the pins.
                                     */
            .mirror_by_cmd = 1,     // Set to 0 if `auto_del_panel_io` is enabled
        },
    };
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_LCD_IO_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = EXAMPLE_LCD_BIT_PER_PIXEL,
        .vendor_config = &vendor_config,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7701(io_handle, &panel_config, &lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(lcd_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(lcd_handle));
    esp_lcd_panel_disp_on_off(lcd_handle, true);

    esp_lcd_touch_handle_t tp_handle = NULL;

    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    const esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG();

    ESP_LOGI(TAG, "Initialize I2C panel IO");
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c((esp_lcd_i2c_bus_handle_t)TOUCH_HOST, &tp_io_config, &tp_io_handle));

    ESP_LOGI(TAG, "Initialize touch controller GT911");
    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = EXAMPLE_LCD_H_RES,
        .y_max = EXAMPLE_LCD_V_RES,
        .rst_gpio_num = EXAMPLE_PIN_NUM_TOUCH_RST,
        .int_gpio_num = EXAMPLE_PIN_NUM_TOUCH_INT,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_gt911(tp_io_handle, &tp_cfg, &tp_handle));
    ESP_ERROR_CHECK(lvgl_port_init(lcd_handle, tp_handle));

    esp_lcd_rgb_panel_event_callbacks_t cbs = {
#if EXAMPLE_RGB_BOUNCE_BUFFER_SIZE > 0
        .on_bounce_frame_finish = rgb_lcd_on_vsync_event,
#else
        .on_vsync = rgb_lcd_on_vsync_event,
#endif
    };
    esp_lcd_rgb_panel_register_event_callbacks(lcd_handle, &cbs, NULL);
#endif
    // Initialize N2K
    ESP_LOGI(TAG, "Initialize N2K");
    OwnN2KInit();
    ESP_LOGI(TAG, "Initialize N2K done");

    // Initialize N2K
    ESP_LOGI(TAG, "Initialize OneWire");
    OneWireInit();
    ESP_LOGI(TAG, "Initialize OneWire done");

    #ifdef INCLUDE_DISPLAY
    // Lock the mutex due to the LVGL APIs are not thread-safe
    if (lvgl_port_lock(-1))
    {
        doLvglInit();
        // Release the mutex
        lvgl_port_unlock();
    } // end if
    #endif
    startUpDelayDone=true;
} //end app_main

static void ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 4 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = LEDC_OUTPUT_IO,
        .duty           = LEDC_DUTY_0, // Set initial duty to 0%
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
} // end ledc init

