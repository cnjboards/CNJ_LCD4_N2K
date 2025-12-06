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
#include "Globals.h"
#include "esp_lcd_panel_ops.h"
#include "driver/ledc.h"
#include "waveshare-display.h"
#include "statusBar.h"

static const char *TAG = "CNJ_LCD4_WS_DISP_lib";
uint32_t blState=0;

// lvgl styls
//static lv_style_t border_style;
static lv_style_t screen1_style;
static lv_style_t screen2_style;
static lv_style_t screen3_style;
static lv_style_t screen4_style;
static lv_style_t screen5_style;
static lv_style_t largeGuageStyle;
static lv_style_t smallGuageStyle;
static lv_style_t indicator_style;
static lv_style_t smallIndicator_style;

// timers for updating the screen
static lv_timer_t *updateScreenTimer;
static lv_timer_t *updateStatusBarTimer;

// lvgl objects for the various screens
// screen 1 and the various screen objects
static lv_obj_t *Screen1;
static lv_obj_t *engineRpmGauge;
static lv_meter_indicator_t *engineRpmIndic;
static lv_obj_t *engineOilGauge;
static lv_meter_indicator_t *engineOilIndic;
static lv_obj_t *temperatureGuage;
static lv_meter_indicator_t *temperatureIndic;
static lv_obj_t *temperatureIndicator;
static lv_obj_t *alternatorGuage;
static lv_meter_indicator_t *alternatorIndic;
static lv_obj_t *alternatorIndicator;
static lv_obj_t *engineRPMIndicator;
static lv_obj_t *engineRPMText;
static lv_obj_t *engineOilIndicator;
// guage graphic objects on engine screen
static lv_obj_t *engineOk;
static lv_obj_t *engineCheck;

// screen 2 and the various screen objects
static lv_obj_t *Screen2;

// screen 3 and the various screen objects
static lv_obj_t *Screen3;

// screen 4 and the various screen objects
static lv_obj_t *Screen4;

// screen 5 and the various screen objects
static lv_obj_t *Screen5;

// used for screen navigation selection list
static lv_obj_t *screenList;
static lv_obj_t *screen2Text;
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
static void setStyle();
static void buildScreen1(void);
static void buildScreen2(void);
static void buildScreen3(void);
static void buildScreen4(void);
static void buildScreen5(void);
static void buildRPMGuage();
static void buildOilGuage();
static void buildTempGuageRound();
static void buildAlternatorGuageRound();
static void updateScreen(lv_timer_t *);
static void updateScreen1();
static void buildScreenList(lv_obj_t *);

// handle screen event
static void event_cb(lv_event_t * e)
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
    // bl state is true
    blState=1;
    // Set duty
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, LEDC_DUTY_100));
    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));        

} // end do_lvgl_init

// setup all lvgl styles
static void setStyle() {
  
  // screen styles
  lv_style_init(&screen1_style);
  lv_style_set_bg_color(&screen1_style, lv_color_black());
  lv_style_set_text_font(&screen1_style, &lv_font_montserrat_20);

  lv_style_init(&screen2_style);
  lv_style_set_bg_color(&screen2_style, lv_color_black());
  lv_style_set_text_font(&screen2_style, &lv_font_montserrat_20);

  lv_style_init(&screen3_style);
  lv_style_set_bg_color(&screen3_style, lv_color_black());
  lv_style_set_text_font(&screen3_style, &lv_font_montserrat_20);

  lv_style_init(&screen4_style);
  lv_style_set_bg_color(&screen4_style, lv_color_black());
  lv_style_set_text_font(&screen4_style, &lv_font_montserrat_20);

  lv_style_init(&screen5_style);
  lv_style_set_bg_color(&screen5_style, lv_color_black());
  lv_style_set_text_font(&screen5_style, &lv_font_montserrat_20);

  // large meter style
  lv_style_init(&largeGuageStyle);
  lv_style_set_bg_color(&largeGuageStyle, lv_color_hex(0x808080));
  lv_style_set_border_color(&largeGuageStyle, lv_color_black());
  lv_style_set_bg_opa(&largeGuageStyle, 100);
  lv_style_set_text_color(&largeGuageStyle, lv_color_white());
  lv_style_set_text_font(&largeGuageStyle, &lv_font_montserrat_20);

  // large meter style
  lv_style_init(&smallGuageStyle);
  lv_style_set_bg_color(&smallGuageStyle, lv_color_hex(0x808080));
  lv_style_set_border_color(&smallGuageStyle, lv_color_black());
  lv_style_set_bg_opa(&smallGuageStyle, 75);
  lv_style_set_text_color(&smallGuageStyle, lv_color_white());
  lv_style_set_text_font(&smallGuageStyle, &lv_font_montserrat_14);

  // style for indicator readouts
  lv_style_init(&indicator_style);
  lv_style_set_border_width(&indicator_style, 4);
  lv_style_set_radius(&indicator_style,4);
  lv_style_set_border_color(&indicator_style, lv_color_black()/*lv_palette_main(LV_PALETTE_GREY)*/);
  lv_style_set_text_font(&indicator_style, &lv_font_montserrat_20);

  // style for indicator readouts
  lv_style_init(&smallIndicator_style);
  lv_style_set_border_width(&smallIndicator_style, 2);
  lv_style_set_radius(&smallIndicator_style,2);
  lv_style_set_border_color(&smallIndicator_style, lv_color_black());
  lv_style_set_text_font(&indicator_style, &lv_font_montserrat_12);

  // init style elements for status bar
  initStatusStyle();

} // end setstyle

// screen 1 builder
static void buildScreen1(void) {
  // main screen frame
  Screen1 = lv_obj_create(NULL);
  lv_obj_add_style(Screen1, &screen1_style, 0);
  lv_obj_set_size(Screen1, LVGL_PORT_H_RES, LVGL_PORT_V_RES);
  lv_obj_align(Screen1, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_scrollbar_mode(Screen1, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(Screen1, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(Screen1, event_cb, LV_EVENT_PRESSED, NULL);
  
  // add status bar
  buildStatusBar(Screen1);
  
  // Components on screen 1
  buildRPMGuage();
  buildOilGuage();
  buildTempGuageRound();
  buildAlternatorGuageRound();

} // end buildScreen1

// screen 2 builder
static void buildScreen2(void) {
  // main screen frame
  Screen2 = lv_obj_create(NULL);
  lv_obj_add_style(Screen2, &screen2_style, 0);
  lv_obj_set_size(Screen2, LVGL_PORT_H_RES, LVGL_PORT_V_RES);
  lv_obj_align(Screen2, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_scrollbar_mode(Screen2, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(Screen2, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(Screen2, event_cb, LV_EVENT_PRESSED, NULL);

  // Build screen 2 components here
  // TBD
  // placeholder label
  screen2Text = lv_label_create(Screen2);
  lv_obj_align_to(screen2Text, Screen2, LV_ALIGN_CENTER, -50, 25);
  lv_obj_set_style_text_font(screen2Text, &lv_font_montserrat_28, 0);
  lv_label_set_text(screen2Text,SCREEN_2_NAME);

} // end buildScreen2

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

  // Build screen 3 components here
  // TBD
  // placeholder label
  screen3Text = lv_label_create(Screen3);
  lv_obj_align_to(screen3Text, Screen3, LV_ALIGN_CENTER, -50, 25);
  lv_obj_set_style_text_font(screen3Text, &lv_font_montserrat_28, 0);
  lv_label_set_text(screen3Text,SCREEN_3_NAME);

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

// locate engine rpm guage on the main screen relative to centre of Screen1 object
// +ve Y is down
// +ve X is right
#define RPM_GUAGE_HIEGHT 440
#define RPM_GUAGE_WIDTH 440
#define ENGINE_RPM_GUAGE_XOFFSET -155
#define ENGINE_RPM_GUAGE_YOFFSET -130
#define ENGINE_LIGHT_X 0
#define ENGINE_LIGHT_Y 195

// build the rpm guage on the main screen
static void buildRPMGuage() {
  // create and rpm guage on main display
  engineRpmGauge = lv_meter_create(Screen1);
  lv_obj_add_style(engineRpmGauge, &largeGuageStyle, 0);

  // declare images for check engine
  LV_IMG_DECLARE(engine_ok); // basically this finds external ref
  LV_IMG_DECLARE(engine_check); // basically this finds external ref
 
  // check engine stuff - OK
  engineOk = lv_img_create(engineRpmGauge);
  lv_obj_add_flag(engineOk, LV_OBJ_FLAG_HIDDEN);
  lv_img_set_src(engineOk, &engine_ok);
  lv_obj_align(engineOk, LV_ALIGN_CENTER, ENGINE_LIGHT_X, ENGINE_LIGHT_Y);
  lv_obj_set_size(engineOk, engine_ok.header.w, engine_ok.header.h);

  // check engine stuff - Check
  engineCheck = lv_img_create(engineRpmGauge);
  lv_obj_add_flag(engineCheck, LV_OBJ_FLAG_HIDDEN);
  lv_img_set_src(engineCheck, &engine_check);
  lv_obj_align(engineCheck, LV_ALIGN_CENTER, ENGINE_LIGHT_X, ENGINE_LIGHT_Y);
  lv_obj_set_size(engineCheck, engine_check.header.w, engine_check.header.h);

  // Locate and set size of rpm guage
  lv_obj_align_to(engineRpmGauge, Screen1, LV_ALIGN_CENTER, ENGINE_RPM_GUAGE_XOFFSET, ENGINE_RPM_GUAGE_YOFFSET);
  lv_obj_set_size(engineRpmGauge, RPM_GUAGE_WIDTH, RPM_GUAGE_HIEGHT);
  lv_obj_add_event_cb(engineRpmGauge, event_cb, LV_EVENT_PRESSED, NULL);

  /*Add a scale first*/
  lv_meter_scale_t * scale = lv_meter_add_scale(engineRpmGauge);
  lv_meter_set_scale_range(engineRpmGauge, scale, 0, 50, 210, 165);
  lv_meter_set_scale_ticks(engineRpmGauge, scale, 51, 4, 20, lv_palette_main(LV_PALETTE_GREY));
  lv_meter_set_scale_major_ticks(engineRpmGauge, scale, 10, 6, 30, lv_color_black(), 15);
  
  /* Green range - arc */
  engineRpmIndic = lv_meter_add_arc(engineRpmGauge, scale, 3, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_meter_set_indicator_start_value(engineRpmGauge, engineRpmIndic, 0);
  lv_meter_set_indicator_end_value(engineRpmGauge, engineRpmIndic, 30);

  /* Green range - ticks */
  engineRpmIndic = lv_meter_add_scale_lines(engineRpmGauge, scale, lv_palette_main(LV_PALETTE_GREEN), lv_palette_main(LV_PALETTE_GREEN),false, 0);
  lv_meter_set_indicator_start_value(engineRpmGauge, engineRpmIndic, 0);
  lv_meter_set_indicator_end_value(engineRpmGauge, engineRpmIndic, 30);

  /* Red range - arc */
  engineRpmIndic = lv_meter_add_arc(engineRpmGauge, scale, 3, lv_palette_main(LV_PALETTE_BLUE), 0);
  lv_meter_set_indicator_start_value(engineRpmGauge, engineRpmIndic, 30);
  lv_meter_set_indicator_end_value(engineRpmGauge, engineRpmIndic, 50);

  /* Red range - arc */
  engineRpmIndic = lv_meter_add_scale_lines(engineRpmGauge, scale, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false,0);
  lv_meter_set_indicator_start_value(engineRpmGauge, engineRpmIndic, 35);
  lv_meter_set_indicator_end_value(engineRpmGauge, engineRpmIndic, 50);

  /* Add the indicator needle and set initial value*/
  engineRpmIndic = lv_meter_add_needle_line(engineRpmGauge, scale, 6, lv_palette_main(LV_PALETTE_BLUE_GREY), -60);
  lv_meter_set_indicator_value(engineRpmGauge, engineRpmIndic, 0);

  /* some static text on the display */
  engineRPMText = lv_label_create(engineRpmGauge);
  lv_obj_align_to(engineRPMText, engineRpmGauge, LV_ALIGN_CENTER, -10, -145);
  lv_obj_set_style_text_font(engineRPMText, &lv_font_montserrat_24, 0);
  lv_label_set_text(engineRPMText,"RPM \nx100");

  /* display the rpm in numerical format as well */
  engineRPMIndicator = lv_label_create(engineRpmGauge);
  lv_obj_add_style(engineRPMIndicator, &indicator_style, 0);
  lv_obj_align_to(engineRPMIndicator, engineRpmGauge, LV_ALIGN_CENTER, -130, -20);
  lv_obj_set_style_text_font(engineRPMIndicator, &lv_font_montserrat_34, 0);
  lv_label_set_text_fmt(engineRPMIndicator, " %04d ", 0);

} // end buildrpmguage

// locate engine oil guage on the main screen relative to centre of Screen1 object
// +ve Y is down
// +ve X is right
#define ENGINE_OIL_GUAGE_SIZE 135
#define ENGINE_OIL_GUAGE_XOFFSET 70
#define ENGINE_OIL_GUAGE_YOFFSET 0
#define INDICATOR_YOFFSET 25

// small guage spacing
#define SMALL_GUAGE_XOFFSET - (ENGINE_OIL_GUAGE_SIZE + 15)
#define SMALL_GUAGE_YOFFSET 145
#define LABEL_YOFFSET 68

// build the oil pressure on the main screen
static void buildOilGuage() {
  // create oil guage on main display
  engineOilGauge = lv_meter_create(Screen1);
  lv_obj_add_style(engineOilGauge, &smallGuageStyle, 0);
  lv_obj_remove_style(engineOilGauge, NULL, LV_PART_MAIN);
  lv_obj_set_style_text_color(engineOilGauge, lv_color_white(), 0);
  // set the size
  lv_obj_set_size(engineOilGauge, ENGINE_OIL_GUAGE_SIZE, ENGINE_OIL_GUAGE_SIZE);
  lv_obj_align_to(engineOilGauge, Screen1, LV_ALIGN_CENTER, ENGINE_OIL_GUAGE_XOFFSET, ENGINE_OIL_GUAGE_YOFFSET);
  lv_obj_add_event_cb(engineOilGauge, event_cb, LV_EVENT_PRESSED, NULL);

  /*Add a scale first*/
  lv_meter_scale_t * scale = lv_meter_add_scale(engineOilGauge);
  lv_meter_set_scale_range(engineOilGauge, scale, 0, 80, 180, 180);
  lv_meter_set_scale_ticks(engineOilGauge, scale, 9, 3, 6, lv_color_black());
  lv_meter_set_scale_major_ticks(engineOilGauge, scale, 2, 5, 8, lv_color_black(), 13);
  
  /* Green range - arc */
  engineOilIndic = lv_meter_add_arc(engineOilGauge, scale, 2, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_meter_set_indicator_start_value(engineOilGauge, engineOilIndic, 20);
  lv_meter_set_indicator_end_value(engineOilGauge, engineOilIndic, 80);

  /* Green range - ticks */
  engineOilIndic = lv_meter_add_scale_lines(engineOilGauge, scale, lv_palette_main(LV_PALETTE_GREEN), lv_palette_main(LV_PALETTE_GREEN),false, 0);
  lv_meter_set_indicator_start_value(engineOilGauge, engineOilIndic, 20);
  lv_meter_set_indicator_end_value(engineOilGauge, engineOilIndic, 80);

  /* Red range - arc */
  engineOilIndic = lv_meter_add_arc(engineOilGauge, scale, 2, lv_palette_main(LV_PALETTE_RED), 0);
  lv_meter_set_indicator_start_value(engineOilGauge, engineOilIndic, 0);
  lv_meter_set_indicator_end_value(engineOilGauge, engineOilIndic, 20);

  /* Red range - arc */
  engineOilIndic = lv_meter_add_scale_lines(engineOilGauge, scale, lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_RED), false,0);
  lv_meter_set_indicator_start_value(engineOilGauge, engineOilIndic, 0);
  lv_meter_set_indicator_end_value(engineOilGauge, engineOilIndic, 20);

  /* Add the indicator needle and set initial value*/
  engineOilIndic = lv_meter_add_needle_line(engineOilGauge, scale, 3, lv_palette_main(LV_PALETTE_BLUE_GREY), -25);
  lv_meter_set_indicator_value(engineOilGauge, engineOilIndic, 0);

  /* display the rpm in numerical format as well */
  engineOilIndicator = lv_label_create(engineOilGauge);
  lv_obj_add_style(engineOilIndicator, &smallIndicator_style, 0);
  lv_obj_align_to(engineOilIndicator, engineOilGauge, LV_ALIGN_CENTER, -8, INDICATOR_YOFFSET);
  lv_obj_set_style_text_font(engineOilIndicator, &lv_font_montserrat_14, 0);
  lv_label_set_text_fmt(engineOilIndicator, " %03d Psi ", 0);

  /* add a label */
  static lv_obj_t *engineOilTitleText = lv_label_create(Screen1);
  lv_obj_align_to(engineOilTitleText, Screen1, LV_ALIGN_CENTER, 55, -72);
  lv_obj_set_style_text_color(engineOilTitleText, lv_color_white(), 0);
  lv_obj_set_style_text_font(engineOilTitleText, &lv_font_montserrat_12, 0);
  lv_label_set_text(engineOilTitleText, "Oil Pressure");

} // end buildoilguage

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
          esp_restart();     
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

// highest guage value
#define MAX_TEMP_RANGE 360

// Temperature guage
static void buildTempGuageRound() {
  // create guage
  temperatureGuage = lv_meter_create(Screen1);
  lv_obj_add_style(temperatureGuage, &smallGuageStyle, 0);
  lv_obj_remove_style(temperatureGuage, NULL, LV_PART_MAIN);
  lv_obj_set_style_text_color(temperatureGuage, lv_color_white(), 0);
  // set the size
  lv_obj_set_size(temperatureGuage, ENGINE_OIL_GUAGE_SIZE, ENGINE_OIL_GUAGE_SIZE);
  lv_obj_align_to(temperatureGuage, Screen1, LV_ALIGN_CENTER, ENGINE_OIL_GUAGE_XOFFSET+SMALL_GUAGE_XOFFSET, ENGINE_OIL_GUAGE_YOFFSET+SMALL_GUAGE_YOFFSET);
  lv_obj_add_event_cb(temperatureGuage, event_cb, LV_EVENT_PRESSED, NULL);

  /*Add a scale */
  lv_meter_scale_t * scale = lv_meter_add_scale(temperatureGuage);
  lv_meter_set_scale_range(temperatureGuage, scale, 0, MAX_TEMP_RANGE, 180, 180);
  lv_meter_set_scale_ticks(temperatureGuage, scale, 5, 3, 7, lv_color_black());
  lv_meter_set_scale_major_ticks(temperatureGuage, scale, 2, 5, 10, lv_color_black(), 13);
  
  /* Green range - arc */
  temperatureIndic = lv_meter_add_arc(temperatureGuage, scale, 2, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_meter_set_indicator_start_value(temperatureGuage, temperatureIndic, 0);
  lv_meter_set_indicator_end_value(temperatureGuage, temperatureIndic, 220);

  /* Green range - ticks */
  temperatureIndic = lv_meter_add_scale_lines(temperatureGuage, scale, lv_palette_main(LV_PALETTE_GREEN), lv_palette_main(LV_PALETTE_GREEN),false, 0);
  lv_meter_set_indicator_start_value(temperatureGuage, temperatureIndic, 0);
  lv_meter_set_indicator_end_value(temperatureGuage, temperatureIndic, 220);

  /* Red range - arc */
  temperatureIndic = lv_meter_add_arc(temperatureGuage, scale, 2, lv_palette_main(LV_PALETTE_RED), 0);
  lv_meter_set_indicator_start_value(temperatureGuage, temperatureIndic, 220);
  lv_meter_set_indicator_end_value(temperatureGuage, temperatureIndic, MAX_TEMP_RANGE);

  /* Red range - arc */
  temperatureIndic = lv_meter_add_scale_lines(temperatureGuage, scale, lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_RED), false,0);
  lv_meter_set_indicator_start_value(temperatureGuage, temperatureIndic, 220);
  lv_meter_set_indicator_end_value(temperatureGuage, temperatureIndic, MAX_TEMP_RANGE);

  /* Add the indicator needle and set initial value*/
  temperatureIndic = lv_meter_add_needle_line(temperatureGuage, scale, 3, lv_palette_main(LV_PALETTE_BLUE_GREY), -25);
  lv_meter_set_indicator_value(temperatureGuage, temperatureIndic, 0);

  /* display the temperature in text format as well */
  temperatureIndicator = lv_label_create(temperatureGuage);
  lv_obj_add_style(temperatureIndicator, &smallIndicator_style, 0);
  lv_obj_align_to(temperatureIndicator, temperatureGuage, LV_ALIGN_CENTER, -15, INDICATOR_YOFFSET);
  lv_obj_set_style_text_font(temperatureIndicator, &lv_font_montserrat_14, 0);
  lv_label_set_text_fmt(temperatureIndicator, " %03d degF ", 0);

  /* add a generic label for the guage, based on screen1 */
  static lv_obj_t *guageText = lv_label_create(Screen1);
  lv_obj_set_style_text_color(guageText, lv_color_white(), 0);
  lv_obj_align_to(guageText, Screen1, LV_ALIGN_CENTER, -100, LABEL_YOFFSET);
  lv_obj_set_style_text_font(guageText, &lv_font_montserrat_12, 0);
  lv_label_set_text(guageText, "Engine Temp");
} // end buildTempGuageRound

#define MAX_VOLTAGE_RANGE 150
// Alternator guage
static void buildAlternatorGuageRound() {
  // create guage
  alternatorGuage = lv_meter_create(Screen1);
  lv_obj_add_style(alternatorGuage, &smallGuageStyle, 0);
  lv_obj_remove_style(alternatorGuage, NULL, LV_PART_MAIN);
  lv_obj_set_style_text_color(alternatorGuage, lv_color_white(), 0);
  // set the size
  lv_obj_set_size(alternatorGuage, ENGINE_OIL_GUAGE_SIZE, ENGINE_OIL_GUAGE_SIZE);
  lv_obj_align_to(alternatorGuage, Screen1, LV_ALIGN_CENTER, ENGINE_OIL_GUAGE_XOFFSET+10, ENGINE_OIL_GUAGE_YOFFSET+SMALL_GUAGE_YOFFSET);
  lv_obj_add_event_cb(alternatorGuage, event_cb, LV_EVENT_PRESSED, NULL);

  /*Add a scale */
  lv_meter_scale_t * scale = lv_meter_add_scale(alternatorGuage);
  lv_meter_set_scale_range(alternatorGuage, scale, 90, MAX_VOLTAGE_RANGE, 180, 180);
  lv_meter_set_scale_ticks(alternatorGuage, scale, 9, 3, 7, lv_color_black());
  lv_meter_set_scale_major_ticks(alternatorGuage, scale, 2, 5, 10, lv_color_black(), 13);
  
  /* Green range - arc */
  alternatorIndic = lv_meter_add_arc(alternatorGuage, scale, 2, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_meter_set_indicator_start_value(alternatorGuage, alternatorIndic, 120);
  lv_meter_set_indicator_end_value(alternatorGuage, alternatorIndic, MAX_VOLTAGE_RANGE);

  /* Green range - ticks */
  alternatorIndic = lv_meter_add_scale_lines(alternatorGuage, scale, lv_palette_main(LV_PALETTE_GREEN), lv_palette_main(LV_PALETTE_GREEN),false, 0);
  lv_meter_set_indicator_start_value(alternatorGuage, alternatorIndic, 120);
  lv_meter_set_indicator_end_value(alternatorGuage, alternatorIndic, MAX_VOLTAGE_RANGE);

  /* Red range - arc */
  alternatorIndic = lv_meter_add_arc(alternatorGuage, scale, 2, lv_palette_main(LV_PALETTE_RED), 0);
  lv_meter_set_indicator_start_value(alternatorGuage, alternatorIndic, 90);
  lv_meter_set_indicator_end_value(alternatorGuage, alternatorIndic, 120);

  /* Red range - arc */
  alternatorIndic = lv_meter_add_scale_lines(alternatorGuage, scale, lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_RED), false,0);
  lv_meter_set_indicator_start_value(alternatorGuage, alternatorIndic, 90);
  lv_meter_set_indicator_end_value(alternatorGuage, alternatorIndic, 120);

  /* Add the indicator needle and set initial value*/
  alternatorIndic = lv_meter_add_needle_line(alternatorGuage, scale, 3, lv_palette_main(LV_PALETTE_BLUE_GREY), -25);
  lv_meter_set_indicator_value(alternatorGuage, alternatorIndic, 0);

  /* display the voltage in text format as well */
  alternatorIndicator = lv_label_create(alternatorGuage);
  lv_obj_add_style(alternatorIndicator, &smallIndicator_style, 0);
  lv_obj_align_to(alternatorIndicator, alternatorGuage, LV_ALIGN_CENTER, -15, INDICATOR_YOFFSET);
  lv_obj_set_style_text_font(alternatorIndicator, &lv_font_montserrat_14, 0);
  lv_label_set_text_fmt(alternatorIndicator, " %04.1f Volts ", 0.0);

  /* add a generic label */
  static lv_obj_t *guageText = lv_label_create(Screen1);
  lv_obj_align_to(guageText, Screen1, LV_ALIGN_CENTER, 50, LABEL_YOFFSET);
  lv_obj_set_style_text_color(guageText, lv_color_white(), 0);
  lv_obj_set_style_text_font(guageText, &lv_font_montserrat_12, 0);
  lv_label_set_text(guageText, "Alternator Volts");
} // end buildAlternatorGuage

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
      // process screen 2
  } else if (active_screen == Screen3) {
      // process screen 3 
  }else if (active_screen == Screen4) {
      // process screen 4
  }else if (active_screen == Screen5) {
      // process screen 5
  } // end if
} // end updateScreen

// Screen 1 refresh handler
// called from timer function when screen1 is active
static void updateScreen1()
{
    // needle is rpm/100 since guage is in 100's of rpm
    lv_meter_set_indicator_value(engineRpmGauge, engineRpmIndic, (uint32_t)(locEngRPM/100));
    // display engine rpm
    lv_label_set_text_fmt(engineRPMIndicator, " %04ld ", (uint32_t)(locEngRPM));
    
    // update check engine light, first bit of status word 1
    if ((locStat1.Bits.CheckEngine == true)) {
      lv_obj_clear_flag(engineCheck, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(engineOk, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_clear_flag(engineOk, LV_OBJ_FLAG_HIDDEN);
      lv_obj_add_flag(engineCheck, LV_OBJ_FLAG_HIDDEN);
    }

    // needle is oil pressure psi
    uint32_t locOilP = (uint32_t)(locEngOilPres/6894.75);
    lv_meter_set_indicator_value(engineOilGauge, engineOilIndic, locOilP);
    // display oil pressure psi
    lv_label_set_text_fmt(engineOilIndicator, " %03d Psi ", (int)locOilP);

    // display engine temperature
    int32_t locEngTemp = (int32_t)((((locEngCoolTemp-273.15)*(9.0/5.0))+32.0));    
    lv_meter_set_indicator_value(temperatureGuage, temperatureIndic, locEngTemp);
    lv_label_set_text_fmt(temperatureIndicator, " %03d degF ", (int)locEngTemp);

    // display alternator voltage
    int32_t locEngVoltage = (int32_t)(locEngAltVolt * 10);
    if (locEngVoltage <= 90)
      locEngVoltage = 90;
    lv_meter_set_indicator_value(alternatorGuage, alternatorIndic, locEngVoltage);      
    lv_label_set_text_fmt(alternatorIndicator, " %04.1f Volts ", locEngAltVolt);

} // end updatescreen1

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
