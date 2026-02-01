// uncomment to enable debugging
// #define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE /* Enable this to show verbose logging for this file only. */

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
#include "esp_sleep.h"
#include "Globals.h"
#include "esp_lcd_panel_ops.h"
#include "driver/ledc.h"
#include "waveshare-display.h"
#include "statusBar.h"
#include "screen1.h"
#include "screen2.h"
#include "styles.h"

static const char *TAG = "CNJ_LCD4_WS_DISP_lib";
uint32_t blState=0;
bool flashBit=0;

// timers for updating the screen
static lv_timer_t *updateScreenTimer;
static lv_timer_t *updateStatusBarTimer;
static lv_timer_t *updateFlasherBitTimer;

// screen 3 and the various screen objects
static lv_obj_t *Screen3;
static lv_obj_t *ui_image;

// screen 4 and the various screen objects
static lv_obj_t *Screen4;

// screen 5 and the various screen objects
static lv_obj_t *Screen5;

// used for screen navigation selection list
static lv_obj_t *screenList;
static lv_obj_t *screen3Text;
static lv_obj_t *screen4Text;
static lv_obj_t *screen5Text;

// list of screen names, simple but works
#define SCREEN_1_NAME "Motoring"
#define SCREEN_2_NAME "Sailing"
#define SCREEN_3_NAME "Tanks"
#define SCREEN_4_NAME "Battery"
#define SCREEN_5_NAME "Configuration"

// forward declarations
static void buildScreen3(void);
static void buildScreen4(void);
static void buildScreen5(void);
static void updateScreen(lv_timer_t *);
static void updateFlashBit(lv_timer_t *);
static void buildScreenList(lv_obj_t *);

// handle screen event
extern "C" void event_cb(lv_event_t * e)
{
  // check if BL was off and turn on if screen touched
  if (blState == 0) {
    // BL was off so turn on and return
    //esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_2, 1);
    esp_lcd_panel_disp_on_off(lcd_handle, true);
    blState=1; // track display state
    // Set duty to 100%
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY_100));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));        
  } else {
    if (lv_obj_has_flag(screenList, LV_OBJ_FLAG_HIDDEN)){
      // unhide
      lv_obj_clear_flag(screenList,LV_OBJ_FLAG_HIDDEN);
    } else {
      // hide
      lv_obj_add_flag(screenList,LV_OBJ_FLAG_HIDDEN);
    } // end if
  } // end else
} // end event_cb

// init lvgl graphics
extern "C" void doLvglInit(){

    // log version    
    ESP_LOGI(TAG, "LVGL: V%i.%i.%i", lv_version_major(), lv_version_minor(), lv_version_patch());

    // start the main UI code here
    setStyle();

    // build all the screens here, for now support 5 screens, dumb ut simple :-)
    buildScreen1();
    buildScreen2();
    buildScreen3();
    buildScreen4();
    buildScreen5();

    // activate main screen (screen 1) on startup
    lv_scr_load(Screen1);
    // build a list for screen navigation and attach to screen 1
    buildScreenList(Screen1);
    // start timer for screen processing
    updateScreenTimer = lv_timer_create(updateScreen, 250, NULL);
    updateStatusBarTimer = lv_timer_create(updateStatusBar, 1500, NULL);
    updateFlasherBitTimer = lv_timer_create(updateFlashBit, 750, NULL);

    // bl state is true
    blState=1;
    // Set duty
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY_100));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));        

} // end do_lvgl_init

// screen 3 builder
static void buildScreen3(void) {
  // main screen frame
  Screen3 = lv_obj_create(NULL);
  lv_obj_add_style(Screen3, &screen3_style, 0);
  lv_obj_set_size(Screen3, LVGL_PORT_H_RES, LVGL_PORT_V_RES);
  lv_obj_align(Screen3, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_scrollbar_mode(Screen3, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(Screen3, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(Screen3, event_cb, LV_EVENT_PRESSED, NULL);

  LV_IMG_DECLARE(Gulls);
 
  // wind guage background
  ui_image = lv_img_create(Screen3);
  //lv_obj_add_flag(engineOk, LV_OBJ_FLAG_HIDDEN);
  lv_img_set_src(ui_image, &Gulls);
  lv_obj_align(ui_image, LV_ALIGN_CENTER, 0,0);
  lv_obj_set_size(ui_image, Gulls.header.w, Gulls.header.h);


  // Build screen 3 components here
  // TBD
  // placeholder label
  //screen3Text = lv_label_create(Screen3);
  //lv_obj_align_to(screen3Text, Screen3, LV_ALIGN_CENTER, -50, 25);
  //lv_obj_set_style_text_font(screen3Text, &lv_font_montserrat_28, 0);
  //lv_label_set_text(screen3Text,SCREEN_3_NAME);

} // end buildScreen3

// screen 4 builder
static void buildScreen4(void) {
  // main screen frame
  Screen4 = lv_obj_create(NULL);
  lv_obj_add_style(Screen4, &screen4_style, 0);
  lv_obj_set_size(Screen4, LVGL_PORT_H_RES, LVGL_PORT_V_RES);
  lv_obj_align(Screen4, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_scrollbar_mode(Screen4, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(Screen4, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(Screen4, event_cb, LV_EVENT_PRESSED, NULL);

  // Build screen 4 components here
  // TBD
  // placeholder label
  screen4Text = lv_label_create(Screen4);
  lv_obj_align_to(screen4Text, Screen4, LV_ALIGN_CENTER, -50, 25);
  lv_obj_set_style_text_font(screen4Text, &lv_font_montserrat_28, 0);
  lv_label_set_text(screen4Text,SCREEN_4_NAME);

} // end buildScreen4

// screen 5 builder
static void buildScreen5(void) {
  // main screen frame
  Screen5 = lv_obj_create(NULL);
  lv_obj_add_style(Screen5, &screen5_style, 0);
  lv_obj_set_size(Screen5, LVGL_PORT_H_RES, LVGL_PORT_V_RES);
  lv_obj_align(Screen5, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_scrollbar_mode(Screen2, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(Screen5, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(Screen5, event_cb, LV_EVENT_PRESSED, NULL);

  // Build screen 5 components here
  // TBD
  // placeholder label
  screen5Text = lv_label_create(Screen5);
  lv_obj_align_to(screen5Text, Screen5, LV_ALIGN_CENTER, -50, 25);
  lv_obj_set_style_text_font(screen5Text, &lv_font_montserrat_28, 0);
  lv_label_set_text(screen5Text,SCREEN_5_NAME);

} // end buildScreen5

// screen list handler - manage screen navigation
static void listEventHandler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target(e);
    if(code == LV_EVENT_CLICKED) {

        /*String selectedItem = String(lv_list_get_btn_text(screenList, obj));*/
        const char* selectedItem = lv_list_get_btn_text(screenList, obj);
        #ifdef SERIALDEBUG
          Serial.printf("Clicked: %s", selectedItem);
        #endif

        if (!strcmp(selectedItem,"BLControl")){
          //esp_io_expander_set_level(io_expander, IO_EXPANDER_PIN_NUM_2, 0);
          esp_lcd_panel_disp_on_off(lcd_handle, false);
          // hide the selction list
          lv_obj_add_flag(screenList, LV_OBJ_FLAG_HIDDEN);
          ESP_LOGI(TAG, "Backlight off ");              
          blState=0;
          // Set duty to 0%
          ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY_0));
          // Update duty to apply the new value
          ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));        

        } else if (!strcmp(selectedItem,"Restart")){
          ESP_LOGI(TAG, "Reset Requested");              
          // reboot
          //esp_restart(); 
          // Reset instantly, preferred over esp_restart
          esp_sleep_enable_timer_wakeup(1);
          esp_deep_sleep_start();    
        // close sleceted so just close the screen list
        } else if (!strcmp(selectedItem,"Close")){
          // hide the list
          lv_obj_add_flag(screenList, LV_OBJ_FLAG_HIDDEN);      
        // screen 1 selected
        } else if (!strcmp(selectedItem,SCREEN_1_NAME)){
          // hide the list
          lv_obj_add_flag(screenList, LV_OBJ_FLAG_HIDDEN);
          // activate Engine Screen
          lv_scr_load(Screen1);
          lv_obj_set_parent(statusBar, Screen1);
          // attach screen list
          lv_obj_set_parent(screenList, Screen1);
        } else if (!strcmp(selectedItem,SCREEN_2_NAME)){
          // hide the list
          lv_obj_add_flag(screenList, LV_OBJ_FLAG_HIDDEN);
          // activate screen 2
          lv_scr_load(Screen2);
          lv_obj_set_parent(statusBar, Screen2);          
          // attach screen list
          lv_obj_set_parent(screenList, Screen2);
        } else if (!strcmp(selectedItem,SCREEN_3_NAME)){
          // hide the list
          lv_obj_add_flag(screenList, LV_OBJ_FLAG_HIDDEN);
          // activate screen 3
          lv_scr_load(Screen3);
          lv_obj_set_parent(statusBar, Screen3);          
          // attach screen list
          lv_obj_set_parent(screenList, Screen3);
        } else if (!strcmp(selectedItem,SCREEN_4_NAME)){
          // hide the list
          lv_obj_add_flag(screenList, LV_OBJ_FLAG_HIDDEN);
          // activate screen 4
          lv_scr_load(Screen4);
          lv_obj_set_parent(statusBar, Screen4);
          // attach screen list
          lv_obj_set_parent(screenList, Screen4);
        } else if (!strcmp(selectedItem,SCREEN_5_NAME)){
          // hide the list
          lv_obj_add_flag(screenList, LV_OBJ_FLAG_HIDDEN);
          // activate screen 5
          lv_scr_load(Screen5);
          lv_obj_set_parent(statusBar, Screen5);
          // attach screen list
          lv_obj_set_parent(screenList, Screen5);
        } // end if
    } // end if
} // end listEventHandler

// function connected to timer to update the active screen periodically
static void updateScreen(lv_timer_t *timer)
{
  // get active screen
  lv_obj_t *active_screen = lv_scr_act();

  // now process the appropriate periodic screen handler
  // based on the active screen
  if (active_screen == Screen1) {
      updateScreen1();
  } else if (active_screen == Screen2) {
      updateScreen2();
  } else if (active_screen == Screen3) {
      // process screen 3 
  }else if (active_screen == Screen4) {
      // process screen 4
  }else if (active_screen == Screen5) {
      // process screen 5
  } // end if
} // end updateScreen

// list to handle screen management
void buildScreenList(lv_obj_t * obj)
{
    /* Create a list */
    screenList = lv_list_create(obj);
    lv_obj_set_size(screenList, lv_pct(60), lv_pct(80));
    lv_obj_center(screenList);
    // hide for now
    lv_obj_add_flag(screenList, LV_OBJ_FLAG_HIDDEN);

    /*Add buttons to the list*/
    lv_obj_t * btn;

    lv_list_add_text(screenList, "Select Screen");
    btn = lv_list_add_btn(screenList, LV_SYMBOL_HOME, SCREEN_1_NAME);
    lv_obj_add_event_cb(btn, listEventHandler, LV_EVENT_CLICKED, NULL);
    btn = lv_list_add_btn(screenList, LV_SYMBOL_IMAGE, SCREEN_2_NAME);
    lv_obj_add_event_cb(btn, listEventHandler, LV_EVENT_CLICKED, NULL);
    btn = lv_list_add_btn(screenList, LV_SYMBOL_IMAGE, SCREEN_3_NAME);
    lv_obj_add_event_cb(btn, listEventHandler, LV_EVENT_CLICKED, NULL);
    btn = lv_list_add_btn(screenList, LV_SYMBOL_IMAGE, SCREEN_4_NAME);
    lv_obj_add_event_cb(btn, listEventHandler, LV_EVENT_CLICKED, NULL);
    btn = lv_list_add_btn(screenList, LV_SYMBOL_SETTINGS, SCREEN_5_NAME);
    lv_obj_add_event_cb(btn, listEventHandler, LV_EVENT_CLICKED, NULL);

    lv_list_add_text(screenList, "Restart");
    btn = lv_list_add_btn(screenList, LV_SYMBOL_POWER, "Restart");
    lv_obj_add_event_cb(btn, listEventHandler, LV_EVENT_CLICKED, NULL);

    lv_list_add_text(screenList, "Backlight Off");
    btn = lv_list_add_btn(screenList, LV_SYMBOL_POWER, "BLControl");
    lv_obj_add_event_cb(btn, listEventHandler, LV_EVENT_CLICKED, NULL);

} // end buildscreenlist

// toggle a bit on a timer for screen indicators
static void updateFlashBit(lv_timer_t *timer) {
  flashBit = !flashBit;
} // used for a flash bit