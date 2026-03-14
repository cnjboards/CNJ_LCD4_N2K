#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE /* Enable this to show verbose logging for this file only. */

#include <lvgl.h>
#include <lvgl_port.h>
#include <string.h>
#include <float.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "driver/ledc.h"
#include "waveshare-display.h"
#include "statusBar.h"
#include "styles.h"
#include "Globals.h"
#include "Screen1.h"
#include "waveshare-display.h"

static const char *TAG = "CNJ_LCD4_SCREEN5";

// screen 5 config screen 
lv_obj_t *Screen5;
lv_obj_t *screen5Btn;

static const char * btnm_map[] = {"Wifi", "Mqtt", "N2K", "\n",
                                  "4", "5", "6", "\n",
                                  "7", "8", "9", "\n",
                                  "Exit", ""
                                 };

static void config_btn_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        uint32_t id = lv_btnmatrix_get_selected_btn(obj);
        const char * txt = lv_btnmatrix_get_btn_text(obj, id);
        ESP_LOGI(TAG, "%s was pressed id=%li\n", txt, id);
        // exit back to screen 1
        if (!strcmp(txt,"Exit")){
          lv_obj_clear_flag(screenList,LV_OBJ_FLAG_HIDDEN);
          // shutdown events for this screen
          //lv_obj_remove_event_cb(screen5Btn, config_btn_handler);
          // flip back to screen 1
          //lv_scr_load(Screen1);
          //lv_obj_set_parent(statusBar, Screen1);
          //lv_obj_add_event_cb(Screen1, event_cb, LV_EVENT_PRESSED, NULL);
        } // end if
    } // end if
} // end button

// screen 2 builder
extern "C" void buildScreen5(void) {

  Screen5 = lv_obj_create(NULL);
  lv_obj_add_style(Screen5, &screen5_style, 0);
  lv_obj_set_size(Screen5, LVGL_PORT_H_RES, LVGL_PORT_V_RES);
  lv_obj_set_scrollbar_mode(Screen5, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(Screen5, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(Screen5, event_cb, LV_EVENT_PRESSED, NULL);
  
  screen5Btn = lv_btnmatrix_create(Screen5);
  lv_obj_add_style(screen5Btn, &screen5_style, 0);
  lv_obj_set_size(screen5Btn, LVGL_PORT_H_RES-75, LVGL_PORT_V_RES-75);
  lv_obj_align(screen5Btn, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_btnmatrix_set_map(screen5Btn, btnm_map);
  lv_btnmatrix_set_btn_width(screen5Btn, 13, 3);
  lv_obj_add_event_cb(screen5Btn, config_btn_handler, LV_EVENT_ALL, NULL);
} // end buildScreen5

// Screen 5 refresh handler
// called from timer function when screen2 is active
extern "C" void updateScreen5()
{
  // do nothing
} // end updateScreen5