// uncomment to enable debugging
// #define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE /* Enable this to show verbose logging for this file only. */

#include <stdio.h>
#include <string.h>
#include <float.h>
#include "esp_log.h"
#include "esp_err.h"
#include <esp_system.h>
#include <sys/param.h>
#include "esp_heap_caps.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_io_expander_tca9554.h"
#include "esp_io_expander.h"
#include "esp_timer.h"
#include "Exio.h"
#include "Globals.h"
#include "display.h"

// map buttons to IO
#define BUTTON1 IO_EXPANDER_PIN_NUM_1
#define BUTTON2 IO_EXPANDER_PIN_NUM_3
#define BUTTON3 IO_EXPANDER_PIN_NUM_7
#define BUTTON4 IO_EXPANDER_PIN_NUM_5
#define BUTTON5 IO_EXPANDER_PIN_NUM_6

static const char *TAG = "CNJ_LCD4_Exio";

// Task handle for expIO handling
TaskHandle_t exioTaskHandle;

esp_io_expander_handle_t io_expander = NULL;
esp_io_expander_handle_t io_expander_ext = NULL;

// forward declarationa
void exioTask (void * parameter);
void sampleExio(void * param);

esp_timer_handle_t exioTimerHandler;
const esp_timer_create_args_t exioTimerArgs = {
            .callback = &sampleExio,
            .name = "Exio Timer"};

uint8_t buttonInputWord;
uint8_t button_history[5];

// this is called periodically to sample button states
// needs to be simple and fast
void sampleExio(void *param)
{
  uint32_t v;
  // get input value
  ESP_ERROR_CHECK(esp_io_expander_get_level(io_expander, (IO_EXPANDER_PIN_NUM_1 | IO_EXPANDER_PIN_NUM_3 | IO_EXPANDER_PIN_NUM_5 | IO_EXPANDER_PIN_NUM_6 | IO_EXPANDER_PIN_NUM_7), &v));
  // make copy for the debounce, invert since input active low
  buttonInputWord = (uint8_t)(~v);
  // track the button history - for now inline ugly, 5 buttons
  button_history[0] = button_history[0] << 1;
  if (buttonInputWord & BUTTON1)
      button_history[0] |= 0x1;

  button_history[1] = button_history[1] << 1;
  if (buttonInputWord & BUTTON2)
      button_history[1] |= 0x1;

  button_history[2] = button_history[2] << 1;
  if (buttonInputWord & BUTTON3)
      button_history[2] |= 0x1;

  button_history[3] = button_history[3] << 1;
  if (buttonInputWord & BUTTON4)
      button_history[3] |= 0x1;

  button_history[4] = button_history[4] << 1;
  if (buttonInputWord & BUTTON5)
      button_history[4] |= 0x1;

} // end sampleExio

// initialize expIO connection and task
extern "C" int initExio(){
    // config the onboard gpio expander
    esp_err_t ret = esp_io_expander_new_i2c_tca9554(TOUCH_HOST, ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_000, &io_expander);

    // config some gpio on expander
    esp_io_expander_set_dir(io_expander, IO_EXPANDER_PIN_NUM_0 | IO_EXPANDER_PIN_NUM_2, IO_EXPANDER_OUTPUT);

    // quick reset pulse for TP and LCD
    esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_0, 0); // TP Reset Exio0
    esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_2, 0); // LCD Reset Exio2
    vTaskDelay(pdMS_TO_TICKS(200));
    esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_0, 1); 
    esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_2, 1);

    // IO for buttons, 5 buttons using Exio P1,P2,P4,P5 and P6
    esp_io_expander_set_dir(io_expander, IO_EXPANDER_PIN_NUM_1 | IO_EXPANDER_PIN_NUM_3 | IO_EXPANDER_PIN_NUM_4 | IO_EXPANDER_PIN_NUM_5 | IO_EXPANDER_PIN_NUM_6, IO_EXPANDER_INPUT);

    // config the external gpio expander - test code
    esp_io_expander_new_i2c_tca9554(TOUCH_HOST, ESP_IO_EXPANDER_I2C_TCA9554_ADDRESS_001, &io_expander_ext);

    // Exio task
    xTaskCreatePinnedToCore (
        exioTask , // Function to implement the task
        "Exio" ,        // Name of the task
        4096 /*configMINIMAL_STACK_SIZE*/,  // Stack size in words
        NULL ,                // Task input parameter
        tskIDLE_PRIORITY + 1, // Priority of the task
        & exioTaskHandle ,    // Task handle.
        1                     // Core where the task should run
    );              

    // small delay to allow task to startup
    vTaskDelay(pdMS_TO_TICKS(200));
    
    // now start a sample timer routine to capture the button states
    ESP_ERROR_CHECK(esp_timer_create(&exioTimerArgs, &exioTimerHandler));
    ESP_ERROR_CHECK(esp_timer_start_periodic(exioTimerHandler, 200)); // this needs to be set >= 200

    // all good so return
    return(ESP_OK);
} // end exioInit()

// task to handle expIO
void exioTask(void * parameter){
    uint32_t ioVal;

    // log message
    ESP_LOGI(TAG, "Starting Exio task");

  // loop forever since we are a task
  for ( ; ; ) {

    // read the input register of exio
    ESP_LOGI(TAG, " Input word: %d History words %d %d %d %d %d", (int)buttonInputWord, button_history[0], button_history[1], button_history[2], button_history[3], button_history[4]);

    // do 500 msec updates, effectively poll expIO every 500 msec
    vTaskDelay(2000);
  } // end for loop

  // should never get here, cleanup if we do
  vTaskDelete(NULL);
} // end expIOTask