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

// screen 2 and the various screen objects
lv_obj_t *Screen2;
static lv_obj_t *backGround;
static lv_obj_t *ui_PanelWindGauge;
static lv_obj_t *ui_ArrowTrue;
static lv_obj_t *ui_ArrowApp;
static lv_obj_t *ui_AppWindSpd;
static lv_obj_t *urIndicator;
static lv_obj_t *ulIndicator;
static lv_obj_t *lrIndicator;
static lv_obj_t *llIndicator;
static lv_obj_t *ui_ulIndicator;
static lv_obj_t *ui_urIndicator;
static lv_obj_t *ui_llIndicator;
static lv_obj_t *ui_lrIndicator;

// screen 2 builder
extern "C" void buildScreen2(void) {
  // main screen frame
  Screen2 = lv_obj_create(NULL);
  lv_obj_add_style(Screen2, &screen2_style, 0);
  lv_obj_set_size(Screen2, LVGL_PORT_H_RES, LVGL_PORT_V_RES);
  lv_obj_align(Screen2, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_scrollbar_mode(Screen2, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(Screen2, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_event_cb(Screen2, event_cb, LV_EVENT_PRESSED, NULL);

  // images for scree
  LV_IMG_DECLARE(scale_back);
  LV_IMG_DECLARE(arrow_app);
  LV_IMG_DECLARE(arrow_true);
  LV_IMG_DECLARE(Indicator);

  // upper left wing indicator with label and value
  #define UL_X -175
  #define UL_Y -150
  // background image for indicator
  ui_ulIndicator = lv_img_create(Screen2);
  lv_img_set_src(ui_ulIndicator, &Indicator);
  lv_obj_align(ui_ulIndicator, LV_ALIGN_CENTER, UL_X, UL_Y);
  lv_obj_set_size(ui_ulIndicator, Indicator.header.w, Indicator.header.h);

  // label for indicator
  static lv_obj_t *ulIndicatorTxt = lv_label_create(ui_ulIndicator);
  lv_obj_add_style(ulIndicatorTxt, &indicator_style_noBorder, 0);
  lv_obj_align_to(ulIndicatorTxt, ui_ulIndicator, LV_ALIGN_CENTER, -25, -25);
  lv_label_set_text_fmt(ulIndicatorTxt, "SOG - Kts");

  // indicator value
  ulIndicator = lv_label_create(ui_ulIndicator);
  lv_obj_add_style(ulIndicator, &largeIndicator_style_noBorder, 0);
  lv_obj_align_to(ulIndicator, ui_ulIndicator, LV_ALIGN_CENTER, 0, +15);
  lv_label_set_text_fmt(ulIndicator, "%04.1f", 0.0);

  // upper right wing indicator with label and value
  #define UR_X -(UL_X-5)
  #define UR_Y UL_Y
  // background image for indicator
  ui_urIndicator = lv_img_create(Screen2);
  lv_img_set_src(ui_urIndicator, &Indicator);
  lv_obj_align(ui_urIndicator, LV_ALIGN_CENTER, UR_X, UR_Y);
  lv_obj_set_size(ui_urIndicator, Indicator.header.w, Indicator.header.h);

  // label for indicator
  static lv_obj_t *urIndicatorTxt = lv_label_create(ui_urIndicator);
  lv_obj_add_style(urIndicatorTxt, &indicator_style_noBorder, 0);
  lv_obj_align_to(urIndicatorTxt, ui_urIndicator, LV_ALIGN_CENTER, -25, -25);
  lv_label_set_text_fmt(urIndicatorTxt, "TWS - Kts");

  // indicator value
  urIndicator = lv_label_create(ui_urIndicator);
  lv_obj_add_style(urIndicator, &largeIndicator_style_noBorder, 0);
  lv_obj_align_to(urIndicator, ui_urIndicator, LV_ALIGN_CENTER, 0, +15);
  lv_label_set_text_fmt(urIndicator, "%04.1f", 0.0);

  // lower left wing indicator with label and value
  #define LL_X UL_X
  #define LL_Y -(UL_Y)
  // background image for indicator
  ui_llIndicator = lv_img_create(Screen2);
  lv_img_set_src(ui_llIndicator, &Indicator);
  lv_obj_align(ui_llIndicator, LV_ALIGN_CENTER, LL_X, LL_Y);
  lv_obj_set_size(ui_llIndicator, Indicator.header.w, Indicator.header.h);

  // label for indicator
  static lv_obj_t *llIndicatorTxt = lv_label_create(ui_llIndicator);
  lv_obj_add_style(llIndicatorTxt, &indicator_style_noBorder, 0);
  lv_obj_align_to(llIndicatorTxt, ui_llIndicator, LV_ALIGN_CENTER, -25, -25);
  lv_label_set_text_fmt(llIndicatorTxt, "STW - Kts");

  // indicator value
  llIndicator = lv_label_create(ui_llIndicator);
  lv_obj_add_style(llIndicator, &largeIndicator_style_noBorder, 0);
  lv_obj_align_to(llIndicator, ui_llIndicator, LV_ALIGN_CENTER, 0, +15);
  lv_label_set_text_fmt(llIndicator, "%04.1f", 0.0);

  // lower right wing indicator with label and value
  #define LR_X UR_X
  #define LR_Y -(UL_Y)
  // background image for indicator
  ui_lrIndicator = lv_img_create(Screen2);
  lv_img_set_src(ui_lrIndicator, &Indicator);
  lv_obj_align(ui_lrIndicator, LV_ALIGN_CENTER, LR_X, LR_Y);
  lv_obj_set_size(ui_lrIndicator, Indicator.header.w, Indicator.header.h);

  // label for indicator
  static lv_obj_t *lrIndicatorTxt = lv_label_create(ui_lrIndicator);
  lv_obj_add_style(lrIndicatorTxt, &indicator_style_noBorder, 0);
  lv_obj_align_to(lrIndicatorTxt, ui_lrIndicator, LV_ALIGN_CENTER, -25, -25);
  lv_label_set_text_fmt(lrIndicatorTxt, "Depth-Ft");

  // indicator value
  lrIndicator = lv_label_create(ui_lrIndicator);
  lv_obj_add_style(lrIndicator, &largeIndicator_style_noBorder, 0);
  lv_obj_align_to(lrIndicator, ui_lrIndicator, LV_ALIGN_CENTER, -8, +15);
  lv_label_set_text_fmt(lrIndicator, "%04d", 0);


  // wind guage background
  ui_PanelWindGauge = lv_img_create(Screen2);
  //lv_obj_add_flag(engineOk, LV_OBJ_FLAG_HIDDEN);
  lv_img_set_src(ui_PanelWindGauge, &scale_back);
  lv_obj_align(ui_PanelWindGauge, LV_ALIGN_CENTER, 0,0);
  lv_obj_set_size(ui_PanelWindGauge, scale_back.header.w, scale_back.header.h);

  // create arrow(s) for true and apparent
  ui_ArrowTrue = lv_img_create(ui_PanelWindGauge);
  lv_img_set_src(ui_ArrowTrue, &arrow_true);
  lv_obj_set_width(ui_ArrowTrue, LV_SIZE_CONTENT); 
  lv_obj_set_height(ui_ArrowTrue, LV_SIZE_CONTENT); 
  lv_obj_set_x(ui_ArrowTrue, 0);
  lv_obj_set_y(ui_ArrowTrue, -130);
  lv_obj_set_align(ui_ArrowTrue, LV_ALIGN_CENTER);
  lv_obj_add_flag(ui_ArrowTrue, LV_OBJ_FLAG_ADV_HITTEST); 
  lv_obj_clear_flag(ui_ArrowTrue, LV_OBJ_FLAG_SCROLLABLE);
  lv_img_set_pivot(ui_ArrowTrue, 8, 148);
  lv_img_set_angle(ui_ArrowTrue, 0);

  ui_ArrowApp = lv_img_create(ui_PanelWindGauge);
  lv_img_set_src(ui_ArrowApp, &arrow_app);
  lv_obj_set_width(ui_ArrowApp, LV_SIZE_CONTENT); 
  lv_obj_set_height(ui_ArrowApp, LV_SIZE_CONTENT); 
  lv_obj_set_x(ui_ArrowApp, 0);
  lv_obj_set_y(ui_ArrowApp, -80);
  lv_obj_set_align(ui_ArrowApp, LV_ALIGN_CENTER);
  lv_obj_add_flag(ui_ArrowApp, LV_OBJ_FLAG_ADV_HITTEST); 
  lv_obj_clear_flag(ui_ArrowApp, LV_OBJ_FLAG_SCROLLABLE); 
  lv_img_set_pivot(ui_ArrowApp, 8, 123);
  lv_img_set_angle(ui_ArrowApp, 0);

  ui_AppWindSpd = lv_label_create(ui_PanelWindGauge);
  lv_obj_set_width(ui_AppWindSpd, LV_SIZE_CONTENT);  
  lv_obj_set_height(ui_AppWindSpd, LV_SIZE_CONTENT); 
  lv_obj_set_x(ui_AppWindSpd, -7);
  lv_obj_set_y(ui_AppWindSpd, 68);
  lv_obj_set_align(ui_AppWindSpd, LV_ALIGN_CENTER);
  lv_obj_set_style_text_color(ui_AppWindSpd, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(ui_AppWindSpd, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(ui_AppWindSpd, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_AppWindSpd, &lv_font_montserrat_36, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(ui_AppWindSpd, "00.0");

} // end buildScreen2

// Screen 2 refresh handler
// called from timer function when screen2 is active
extern "C" void updateScreen2()
{
  char buf[10];

  #ifdef USEN2KDATA
    // update wind direction app
    int32_t tmp = (uint32_t)(locWindAngleApp * 180 / PI);
    lv_img_set_angle(ui_ArrowApp, (tmp * 10));
    // wind dir true
    tmp = (uint32_t)(locWindAngleTrue * 180 / PI);
    lv_img_set_angle(ui_ArrowTrue, (tmp * 10));

    // update wind speed app
    sprintf(buf, "%0.1f", (locWindSpeedApp * 1.944));
    lv_label_set_text(ui_AppWindSpd, buf);

    // update wind speed rrue
    lv_label_set_text_fmt(urIndicator, "%04.1f", (locWindSpeedTrue * 1.944));

    // update sog
    lv_label_set_text_fmt(ulIndicator, "%04.1f", (locSOG * 1.944));

    // update stw
    lv_label_set_text_fmt(llIndicator, "%04.1f", (locSTW * 1.944));

    // update depth
    lv_label_set_text_fmt(lrIndicator, "%04d", (int)(locDepthBelowKeel * 3.28));
  #else
    // update wind direction app
    int32_t tmp = (uint32_t)(mqttWindAngleApp * 180 / PI);
    lv_img_set_angle(ui_ArrowApp, (tmp * 10));
    // wind dir true
    tmp = (uint32_t)(mqttWindAngleTrue * 180 / PI);
    lv_img_set_angle(ui_ArrowTrue, (tmp * 10));

    // update wind speed app
    sprintf(buf, "%0.1f", (mqttWindSpeedApp * 1.944));
    lv_label_set_text(ui_AppWindSpd, buf);

    // update wind speed rrue
    lv_label_set_text_fmt(urIndicator, "%04.1f", (mqttWindSpeedTrue * 1.944));

    // update sog
    lv_label_set_text_fmt(ulIndicator, "%04.1f", (mqttSOG * 1.944));

    // update stw
    lv_label_set_text_fmt(llIndicator, "%04.1f", (mqttSTW * 1.944));

    // update depth
    lv_label_set_text_fmt(lrIndicator, "%04d", (int)(mqttDepthBelowKeel * 3.28));
  #endif
} // end updatescreen2
