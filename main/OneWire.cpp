// uncomment to enable debugging
//#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE /* Enable this to show verbose logging for this file only. */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_check.h"
#include "OneWireNg_CurrentPlatform.h"
#include "drivers/DSTherm.h"
#include "utils/Placeholder.h"
#include "platform/Platform_Delay.h"

#define ONEWIRE_TASK_PRIORITY     15

static const char *TAG = "CNJ_LCD4_OW_lib";
static TaskHandle_t OneWire_task_handle = NULL;

// Onewire stuff
#ifndef CONFIG_OW_PIN
# error "CONFIG_OW_PIN is required"
#endif

#if !defined(CONFIG_SINGLE_SENSOR) && !CONFIG_SEARCH_ENABLED
# error "CONFIG_SEARCH_ENABLED is required for non signle sensor setup"
#endif

#if defined(CONFIG_PWR_CTRL_PIN) && !CONFIG_PWR_CTRL_ENABLED
# error "CONFIG_PWR_CTRL_ENABLED is required if CONFIG_PWR_CTRL_PIN is configured"
#endif

#if (CONFIG_MAX_SEARCH_FILTERS > 0)
static_assert(CONFIG_MAX_SEARCH_FILTERS >= DSTherm::SUPPORTED_SLAVES_NUM,
    "CONFIG_MAX_SEARCH_FILTERS too small");
#endif

#ifdef CONFIG_PARASITE_POWER
# define PARASITE_POWER_ARG true
#else
# define PARASITE_POWER_ARG false
#endif

static Placeholder<OneWireNg_CurrentPlatform> ow;


/* returns false if not supported */
static bool printId(const OneWireNg::Id& id)
{
    const char *name = DSTherm::getFamilyName(id);

    for (size_t i = 0; i < sizeof(OneWireNg::Id); i++)
        printf("%s%02x", (!i ? "" : ":"), id[i]);

    if (name)
        printf(" -> %s", name);

    printf("\n");

    return (name != NULL);
}

static void printScratchpad(const DSTherm::Scratchpad& scrpd)
{
    const uint8_t *scrpd_raw = scrpd.getRaw();

    printf("  Scratchpad:");
    for (size_t i = 0; i < DSTherm::Scratchpad::LENGTH; i++)
        printf("%c%02x", (!i ? ' ' : ':'), scrpd_raw[i]);


    printf("; Th:%d; Tl:%d; Resolution:%d",
        scrpd.getTh(), scrpd.getTl(),
        9 + (int)(scrpd.getResolution() - DSTherm::RES_9_BIT));

    long temp = scrpd.getTemp2();
    printf("; Temp:");
    if (temp < 0) {
        temp = -temp;
        printf("-");
    }
    printf("%d.%04d C\n", (int)temp / 16, (10000 * ((int)temp % 16)) / 16);
}

// This is a FreeRTOS task
void OneWire_task(void *pvParameters)
{
    DSTherm drv(ow);
    /* read sensors one-by-one */
    Placeholder<DSTherm::Scratchpad> scrpd;

    // log message
    ESP_LOGI(TAG, "Starting OneWire task");

    // main onewire loop
    for (;;)
    {
        ESP_LOGV(TAG, "------- OneWire Poll -------");
        /* convert temperature on all sensors connected... */
        drv.convertTempAll(DSTherm::MAX_CONV_TIME, PARASITE_POWER_ARG);

    #ifdef CONFIG_SINGLE_SENSOR
        /* single sensor environment */

        /*
        * Zero-initialized scratchpad placeholder is static to allow reuse of
        * the associated sensor id while reissuing readScratchpadSingle() calls.
        */
        
        OneWireNg::ErrorCode ec = drv.readScratchpadSingle(scrpd);
        if (ec == OneWireNg::EC_SUCCESS) {
            printId(scrpd->getId());
            printScratchpad(scrpd);
        } else if (ec == OneWireNg::EC_CRC_ERROR)
            printf("  CRC error.\n");
    #else
        for (const auto& id: *ow) {
            if (printId(id)) {
                if (drv.readScratchpad(id, scrpd) == OneWireNg::EC_SUCCESS)
                    printScratchpad(scrpd);
                else
                    printf("  Read scratchpad error.\n");
            }
        }
    #endif
        printf("----------\n");

        // do every few seconds or so
        vTaskDelay(2000);
    } // end for

    vTaskDelete(NULL); // should never get here...
} // end n2k task

extern "C" int OneWireInit()
{
    // enable verbose logging in this module
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);

    esp_err_t result = ESP_OK;
#ifdef CONFIG_PWR_CTRL_PIN
    new (&ow) OneWireNg_CurrentPlatform(CONFIG_OW_PIN, CONFIG_PWR_CTRL_PIN, false);
#else
    new (&ow) OneWireNg_CurrentPlatform(CONFIG_OW_PIN, false);
#endif
    DSTherm drv(ow);

#if (CONFIG_MAX_SEARCH_FILTERS > 0)
    drv.filterSupportedSlaves();
#endif

#ifdef CONFIG_COMMON_RES
    /*
     * Set common resolution for all sensors.
     * Th, Tl (high/low alarm triggers) are set to 0.
     */
    drv.writeScratchpadAll(0, 0,
        (DSTherm::Resolution)(DSTherm::RES_9_BIT + (CONFIG_COMMON_RES - 9)));

    /*
     * The configuration above is stored in volatile sensors scratchpad
     * memory and will be lost after power unplug. Therefore store the
     * configuration permanently in sensors EEPROM.
     */
    drv.copyScratchpadAll(PARASITE_POWER_ARG);
#endif
    // start onewire task
    xTaskCreate(
        &OneWire_task,            // Pointer to the task entry function.
        "OneWire_task",           // A descriptive name for the task for debugging.
        2048,                 // size of the task stack in bytes.
        NULL,                 // Optional pointer to pvParameters
        ONEWIRE_TASK_PRIORITY, // priority at which the task should run
        &OneWire_task_handle      // Optional pass back task handle
    );

    // check for issues on create
    if (OneWire_task_handle == NULL)
    {
        ESP_LOGE(TAG, "Unable to create OneWire task.");
        result = ESP_ERR_NO_MEM;
        goto err_out;
    } // end if

err_out:
    // clean up
    if (result != ESP_OK)
    {
        if (OneWire_task_handle != NULL)
        {
            vTaskDelete(OneWire_task_handle);
            OneWire_task_handle = NULL;
        } // end if
    } // end if
    return result;

} // end onwireinit