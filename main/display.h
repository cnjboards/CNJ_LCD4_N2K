#pragma once
#include "esp_lcd_st7701.h"
#include "display.h"
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
#define TOUCH_HOST (I2C_NUM_0)
#define EXAMPLE_PIN_NUM_TOUCH_SCL (GPIO_NUM_7)
#define EXAMPLE_PIN_NUM_TOUCH_SDA (GPIO_NUM_15)
#define EXAMPLE_PIN_NUM_TOUCH_RST (-1)          // -1 if not used
#define EXAMPLE_PIN_NUM_TOUCH_INT (GPIO_NUM_16) // -1 if not used

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
