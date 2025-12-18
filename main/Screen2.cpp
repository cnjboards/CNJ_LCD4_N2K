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
#include "Globals.h"
#include "esp_lcd_panel_ops.h"
#include "driver/ledc.h"
#include "waveshare-display.h"
#include "statusBar.h"
#include "styles.h"

// screen 2 and the various screen objects
lv_obj_t *Screen2;
static lv_obj_t *backGround;
static lv_obj_t *ui_PanelWindGauge;
static lv_obj_t *ui_ArrowTrue;
static lv_obj_t *ui_ArrowApp;
static lv_obj_t *ui_AppWindSpd;

static lv_obj_t *screen2Text;

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

  // Build screen 2 components here
  // TBD
  // declare images for check engine
  LV_IMG_DECLARE(scale_back); // basically this finds external ref
  LV_IMG_DECLARE(arrow_app);
  LV_IMG_DECLARE(arrow_true);
 
  // wind guage background
  ui_PanelWindGauge = lv_img_create(Screen2);
  //lv_obj_add_flag(engineOk, LV_OBJ_FLAG_HIDDEN);
  lv_img_set_src(ui_PanelWindGauge, &scale_back);
  lv_obj_align(ui_PanelWindGauge, LV_ALIGN_CENTER, 0,0);
  lv_obj_set_size(ui_PanelWindGauge, scale_back.header.w, scale_back.header.h);

  // create arrow(s) for true and apparent
  ui_ArrowTrue = lv_img_create(ui_PanelWindGauge);
  lv_img_set_src(ui_ArrowTrue, &arrow_true);
  lv_obj_set_width(ui_ArrowTrue, LV_SIZE_CONTENT);   /// 1
  lv_obj_set_height(ui_ArrowTrue, LV_SIZE_CONTENT);    /// 1
  lv_obj_set_x(ui_ArrowTrue, 0);
  lv_obj_set_y(ui_ArrowTrue, -130);
  lv_obj_set_align(ui_ArrowTrue, LV_ALIGN_CENTER);
  lv_obj_add_flag(ui_ArrowTrue, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
  lv_obj_clear_flag(ui_ArrowTrue, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
  lv_img_set_pivot(ui_ArrowTrue, 8, 148);
  lv_img_set_angle(ui_ArrowTrue, 0);

  ui_ArrowApp = lv_img_create(ui_PanelWindGauge);
  lv_img_set_src(ui_ArrowApp, &arrow_app);
  lv_obj_set_width(ui_ArrowApp, LV_SIZE_CONTENT);   /// 1
  lv_obj_set_height(ui_ArrowApp, LV_SIZE_CONTENT);    /// 1
  lv_obj_set_x(ui_ArrowApp, 0);
  lv_obj_set_y(ui_ArrowApp, -80);
  lv_obj_set_align(ui_ArrowApp, LV_ALIGN_CENTER);
  lv_obj_add_flag(ui_ArrowApp, LV_OBJ_FLAG_ADV_HITTEST);     /// Flags
  lv_obj_clear_flag(ui_ArrowApp, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
  lv_img_set_pivot(ui_ArrowApp, 8, 123);
  lv_img_set_angle(ui_ArrowApp, 0);

  ui_AppWindSpd = lv_label_create(ui_PanelWindGauge);
  lv_obj_set_width(ui_AppWindSpd, LV_SIZE_CONTENT);   /// 1
  lv_obj_set_height(ui_AppWindSpd, LV_SIZE_CONTENT);    /// 1
  lv_obj_set_x(ui_AppWindSpd, -7);
  lv_obj_set_y(ui_AppWindSpd, 68);
  lv_obj_set_align(ui_AppWindSpd, LV_ALIGN_CENTER);
  lv_obj_set_style_text_color(ui_AppWindSpd, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_opa(ui_AppWindSpd, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_align(ui_AppWindSpd, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_obj_set_style_text_font(ui_AppWindSpd, &lv_font_montserrat_36, LV_PART_MAIN | LV_STATE_DEFAULT);
  lv_label_set_text(ui_AppWindSpd, "00.0");

  // placeholder label
  //screen2Text = lv_label_create(Screen2);
  //lv_obj_align_to(screen2Text, Screen2, LV_ALIGN_CENTER, -50, 25);
  //lv_obj_set_style_text_color(screen2Text, lv_color_white(), 0);
  //lv_obj_set_style_text_font(screen2Text, &lv_font_montserrat_28, 0);
  //lv_label_set_text(screen2Text,SCREEN_2_NAME);

} // end buildScreen2

// Screen 2 refresh handler
// called from timer function when screen2 is active
#define PI 3.14159265358979323846
extern "C" void updateScreen2()
{
  char buf[10];
  
  // update wind direction
  int32_t tmp = (uint32_t)(locWindAngleApp * 180 / PI);
  lv_img_set_angle(ui_ArrowApp, (tmp * 10));
  tmp = (uint32_t)(locWindAngleTrue * 180 / PI);
  lv_img_set_angle(ui_ArrowTrue, (tmp * 10));

  // update wind speed
  sprintf(buf, "%0.1f", (locWindSpeedApp * 1.944));
  lv_label_set_text(ui_AppWindSpd, buf);

} // end updatescreen2
