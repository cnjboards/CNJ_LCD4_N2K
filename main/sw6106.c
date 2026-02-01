#define LOG_LOCAL_LEVEL ESP_LOG_WARN

#include <stdio.h>
#include <float.h>
#include <string.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SW6106_TASK_DELAY_MS 2500
#define SW6106_TASK_STACK_SIZE   (4 * 1024)
#define SW6106_TASK_PRIORITY     10
#define SW6106MINBATTPRESENTVOLTAGE 2000 // minimum battery voltage for detecting battery
#define SW6106_VERSION_PRESENT 0x6 // version to look for to detect sw6106

// some i2c defines for comms
#define SW6106I2CADDR 0x3c
#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define WRITE_BIT I2C_MASTER_WRITE  /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ    /*!< I2C master read */
#define ACK_CHECK_EN 0x1            /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0           /*!< I2C master will not check ack from slave */
#define ACK_VAL I2C_MASTER_ACK      /*!< I2C ack value */
#define NACK_VAL I2C_MASTER_NACK    /*!< I2C nack value */

bool lipoPresent = false;
unsigned int soc = 0;
int current = 0;
i2c_port_t locI2cPort = (I2C_NUM_0);
static const char *TAG = "sw6106";

// forward declarations
esp_err_t sw6106_reg_read(uint8_t , uint8_t *);
static void sw6106_task(void *);
esp_err_t sw6106Init(i2c_port_t );
esp_err_t sw6106_reg_write(uint8_t , uint8_t );

// Task to handle sw6106 management
static void sw6106_task(void *arg)
{
    TickType_t xLastWakeTime;
    ESP_LOGI(TAG, "Starting sw6106 task");
    uint8_t data_addr, data, charging;
    uint32_t vBat, iChg;
    esp_err_t ret;

    // put any initialization code here
    // set low charge current detect to off??
    ret = sw6106_reg_write(0x38, 0xA);
    // read back - check
    ret = sw6106_reg_read(0x38, &data);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "sw6106 Dump r=0x38 d=%x", data);
    } else {
        ESP_LOGI(TAG, "sw6106 Dump r=0x38 XX");
    } // end if

    // capture the tick
    xLastWakeTime = xTaskGetTickCount ();
    // the task runs periodically which means the for loop runs periodically
    for ( ; ; ) {

            // read sw6106 status register
            data_addr = 0x11;
            charging=0;
            ret = sw6106_reg_read(data_addr, &data);
            if (ret == ESP_OK) {
                if ((data & 0x10) != 0)
                    // charging
                    charging = 1;
                ESP_LOGI(TAG, "sw6106 Dump status reg (0x11)=%x charging=%x", data, charging);
            } else {
                ESP_LOGI(TAG, "sw6106 Dump r=%x XX", data_addr);
            } // end if

            // read batt level register
            data_addr = 0x4f;
            ret = sw6106_reg_read(data_addr, &data);
            if (ret == ESP_OK) {
                soc = (unsigned int)data;
                ESP_LOGI(TAG, "sw6106 battery SOC=%u Percent", soc);
            } else {
                ESP_LOGI(TAG, "sw6106 Dump r=%x XX", data_addr);
            } // end if

            // read batt level register(s)
            data_addr = 0x15;
            vBat=0;
            ret = sw6106_reg_read(data_addr, &data);            
            if (ret == ESP_OK) {
                data &= 0x0f;
                vBat = (uint32_t)(data);
                vBat <<= 8;
                data_addr = 0x14;
                ret = sw6106_reg_read(data_addr, &data);                        
                if (ret == ESP_OK) {
                    vBat += (uint32_t)(data);
                    vBat = ((vBat * 12)/10);
                    if (vBat > SW6106MINBATTPRESENTVOLTAGE)
                        lipoPresent = true;
                    ESP_LOGI(TAG, "sw6106 vBat=%lu mV", vBat);
                } else {
                    ESP_LOGI(TAG, "sw6106 Dump r=%x XX", data_addr);
                } // end if
            } else {
                ESP_LOGI(TAG, "sw6106 Dump r=%x XX", data_addr);
            } // end if
        
            // read batt level register
            data_addr = 0x18;
            iChg=0;
            ret = sw6106_reg_read(data_addr, &data);                                    
            if (ret == ESP_OK) {
                data &= 0x0f;
                iChg = (uint32_t)(data);
                iChg <<= 8;
                // read batt level register
                data_addr = 0x17;
                ret = sw6106_reg_read(data_addr, &data);                                                
                if (ret == ESP_OK) {
                    iChg += (uint32_t)(data);
                    current = ((iChg * 25)/7);
                    ESP_LOGI(TAG, "sw6106 charge current=%u mA", current);
                } else {
                    ESP_LOGI(TAG, "sw6106 Dump r=%x XX", data_addr);
                } // end if
            } else {
                ESP_LOGI(TAG, "sw6106 Dump r=%x XX", data_addr);
            } // end if
        // Wait for the next cycle.
        vTaskDelayUntil( &xLastWakeTime, pdMS_TO_TICKS(SW6106_TASK_DELAY_MS));
    } // end while
} // end lvgl_task

// initialize the sw6106 driver
esp_err_t sw6106Init(i2c_port_t i2cPort) {
    
    uint8_t data=0;
    esp_err_t ret;

    if (i2cPort >= 0 && i2cPort < I2C_NUM_MAX) {
        // local var to keep i2cPort number in use
        locI2cPort = i2cPort;

        // now detect the sw6106, read the version register, 0x26
        ret = sw6106_reg_read(0x26, &data);
        if (data != SW6106_VERSION_PRESENT) {
            // no chip detected, return
            return ESP_FAIL;
        } // end if

        // create task to manage sw6106 i2c comms
        xTaskCreate(sw6106_task, 
                "SW6106", 
                SW6106_TASK_STACK_SIZE, 
                NULL, 
                SW6106_TASK_PRIORITY, 
                NULL);
        return ESP_OK;
    } else {
        // just return
        return ESP_FAIL;
    } // end if
}// end sw6106Init

// helper routine to read from sw6106
esp_err_t sw6106_reg_read(uint8_t data_addr, uint8_t *data) {
    esp_err_t ret;
    uint8_t locData = 0;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SW6106I2CADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data_addr, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SW6106I2CADDR << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, &locData, NACK_VAL);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(locI2cPort, cmd, 50 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    // check result
    if (ret == ESP_OK) {
        //ESP_LOGI(TAG, "sw6106 reg read r=%x d=%x", data_addr, locData);
        // copy value back
        memcpy(data, &locData, sizeof(locData));
        return ESP_OK;
    } else {
        return ESP_FAIL;
    } // end if
} // end sw6106_reg_read

// helper routine to write to sw6106
esp_err_t sw6106_reg_write(uint8_t data_addr, uint8_t data) {
    esp_err_t ret;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, SW6106I2CADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data_addr, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, data, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(locI2cPort, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
} // end sw6106_reg_write

// power off helper
// if no 12V, unit will power down, use 12V or button to power on
void powerOff(){
    sw6106_reg_write(0x3, 0x10);
} // end poweroff
