// uncomment to enable debugging
// #define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE /* Enable this to show verbose logging for this file only. */

#include <lvgl.h>
#include "lvgl_port.h"
#include <string.h>
#include <float.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "globals.h"

static lv_style_t status_style;
lv_obj_t *statusBar;
static lv_obj_t *wifiLabel;
static lv_obj_t *ipLabel;
static lv_obj_t *ssidLabel;
static lv_obj_t *n2KLabel;

// used to initialize style for common statusBar
extern "C" void initStatusStyle(void){

  // overall style 
  lv_style_init(&status_style);
  lv_style_set_bg_color(&status_style, lv_color_hex(0x808080));
  lv_style_set_border_width(&status_style, 0);
  lv_style_set_border_color(&status_style, lv_color_black());
  lv_style_set_text_font(&status_style, &lv_font_montserrat_14);
 
} // end initStatusStyle

// generic status bar
extern "C" void buildStatusBar(lv_obj_t *parent) {

  // derive the status bar hieght
  lv_coord_t sBarHeight = (LVGL_PORT_V_RES/13);

  // header bar at top of screen
  statusBar = lv_obj_create(parent);
  lv_obj_add_style(statusBar, &status_style, 0);
  lv_obj_set_size(statusBar, LVGL_PORT_H_RES, sBarHeight);
  lv_obj_align(statusBar, LV_ALIGN_TOP_LEFT, 1, 2);
  lv_obj_remove_style(statusBar, NULL, LV_PART_SCROLLBAR | LV_STATE_ANY);

  // wifi indicator
  wifiLabel = lv_label_create(statusBar);
  lv_obj_align(wifiLabel, LV_ALIGN_LEFT_MID, 2, 8);
  lv_obj_set_size(wifiLabel, 70, sBarHeight);
  lv_label_set_text(wifiLabel, "WiFi " LV_SYMBOL_CLOSE);

  // ip address if connected
  ipLabel = lv_label_create(statusBar);
  lv_obj_align(ipLabel, LV_ALIGN_LEFT_MID, 72, 8);
  lv_obj_set_size(ipLabel, 180, sBarHeight);
  lv_label_set_text(ipLabel, "IP: ");

  // ssid if connected
  ssidLabel = lv_label_create(statusBar);
  lv_obj_align(ssidLabel, LV_ALIGN_LEFT_MID, 210, 8);
  lv_obj_set_size(ssidLabel, 140, sBarHeight);
  lv_label_set_text(ssidLabel, "SSID: ");

  // N2K if connected
  n2KLabel = lv_label_create(statusBar);
  lv_obj_align(n2KLabel, LV_ALIGN_LEFT_MID, 350, 8);
  lv_obj_set_size(n2KLabel, 130, sBarHeight);
  lv_style_set_text_color(&status_style, lv_color_make(100,10,10));
  lv_label_set_text(n2KLabel, "N2K: Down");

} // end buildstatusbar

// status bar update routine called periodically
extern "C" void updateStatusBar(lv_timer_t *timer)
{
  char buff[255];
  // update wifi stuff on status bar
  if ((xEventGroupGetBits(s_wifi_event_group)& BIT0)) {
      lv_style_set_text_color(&status_style, lv_color_make(10,100,10));
      lv_label_set_text(wifiLabel, "WiFi " LV_SYMBOL_WIFI);
      // ip address
      sprintf(buff,"IP: %s", myIpAddressSta);
      lv_label_set_text(ipLabel, buff);
      // ssid 
      sprintf(buff,"SSID: %s", wifiStaSsid);
      lv_label_set_text(ssidLabel, buff);
  } else {
      lv_style_set_text_color(&status_style, lv_color_make(100,10,10));
      lv_label_set_text(wifiLabel, "WiFi " LV_SYMBOL_CLOSE);
      // ip address
      sprintf(buff,"IP: <NONE>");
      lv_label_set_text(ipLabel, buff);
      // ssid
      sprintf(buff,"SSID: <NONE>");
      lv_label_set_text(ssidLabel, buff);
  } // end if

  // update N2K stuff on the status bar
  if (n2kUp){
      // display the number of stations and make green
      lv_style_set_text_color(&status_style, lv_color_make(10,100,10));
      sprintf(buff,"N2K: Up Sta=%d", n2kConnected);
      lv_label_set_text(n2KLabel, buff);
  } else {
      // make red
      lv_style_set_text_color(&status_style, lv_color_make(100,10,10));
      sprintf(buff,"N2K: Down");
      lv_label_set_text(n2KLabel, buff);
  } // end if
} // end update status bar
