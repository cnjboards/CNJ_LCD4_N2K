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

// lvgl objects for the various screens
// screen 1 and the various screen objects
lv_obj_t *Screen1;
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
static lv_obj_t *uRIndicator;
static lv_obj_t *uLIndicator;
static lv_obj_t *lRIndicator;
static lv_obj_t *lLIndicator;

// guage graphic objects on engine screen
static lv_obj_t *engineOk;
static lv_obj_t *engineCheck;
static lv_obj_t *oilFlt;
static lv_obj_t *tempFlt;

// forward declarations
static void buildRPMGuage();
static void buildOilGuage();
static void buildTempGuageRound();
static void buildAlternatorGuageRound();

// screen 1 builder
extern "C" void buildScreen1(void) {
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

// Screen 1 refresh handler
// called from timer function when screen1 is active
extern "C" void updateScreen1()
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
    } // end if

    // needle is oil pressure psi
    uint32_t locOilP = (uint32_t)(locEngOilPres/6894.75);
    lv_meter_set_indicator_value(engineOilGauge, engineOilIndic, locOilP);
    // display oil pressure psi
    lv_label_set_text_fmt(engineOilIndicator, " %03d Psi ", (int)locOilP);

    // update low oil pressure light
    if ((locStat1.Bits.LowOilPressure == true)) {
      lv_obj_clear_flag(oilFlt, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(oilFlt, LV_OBJ_FLAG_HIDDEN);
    } // end if

    // display engine temperature
    int32_t locEngTemp = (int32_t)((((locEngCoolTemp-273.15)*(9.0/5.0))+32.0));    
    lv_meter_set_indicator_value(temperatureGuage, temperatureIndic, locEngTemp);
    lv_label_set_text_fmt(temperatureIndicator, " %03d degF ", (int)locEngTemp);

    // update Hi temp light
    if ((locStat1.Bits.OverTemperature == true)) {
      lv_obj_clear_flag(tempFlt, LV_OBJ_FLAG_HIDDEN);
    } else {
      lv_obj_add_flag(tempFlt, LV_OBJ_FLAG_HIDDEN);
    } // end if

    // display alternator voltage
    int32_t locEngVoltage = (int32_t)(locEngAltVolt * 10);
    if (locEngVoltage <= 90)
      locEngVoltage = 90;
    lv_meter_set_indicator_value(alternatorGuage, alternatorIndic, locEngVoltage);      
    lv_label_set_text_fmt(alternatorIndicator, " %04.1f Volts ", locEngAltVolt);

} // end updatescreen1

// locate engine rpm guage on the main screen relative to centre of Screen1 object
// +ve Y is down
// +ve X is right
#define RPM_GUAGE_HIEGHT lv_pct(91)
#define RPM_GUAGE_WIDTH lv_pct(91)
#define ENGINE_RPM_GUAGE_XOFFSET -155
#define ENGINE_RPM_GUAGE_YOFFSET -130
#define ENGINE_LIGHT_X 0
#define ENGINE_LIGHT_Y 195

// build the rpm guage on the main screen
static void buildRPMGuage() {

  // some wing indicators
  uRIndicator = lv_label_create(Screen1);
  //lv_obj_add_flag(uRIndicator, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_style(uRIndicator, &largeIndicator_style, 0);
  lv_obj_align_to(uRIndicator, Screen1, LV_ALIGN_CENTER, -195, -150); // was -175
  lv_label_set_text_fmt(uRIndicator, "%04d", 0);

  uLIndicator = lv_label_create(Screen1);
  //lv_obj_add_flag(uLIndicator, LV_OBJ_FLAG_HIDDEN);  
  lv_obj_add_style(uLIndicator, &largeIndicator_style, 0);
  lv_obj_align_to(uLIndicator, Screen1, LV_ALIGN_CENTER, 165, -150); // was -175
  lv_label_set_text_fmt(uLIndicator, "%04d", 0);

  // some wing indicators
  lRIndicator = lv_label_create(Screen1);
  //lv_obj_add_flag(uRIndicator, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_style(lRIndicator, &largeIndicator_style, 0);
  lv_obj_align_to(lRIndicator, Screen1, LV_ALIGN_CENTER, -195, 222);
  lv_label_set_text_fmt(lRIndicator, "%04d", 0);

  lLIndicator = lv_label_create(Screen1);
  //lv_obj_add_flag(uLIndicator, LV_OBJ_FLAG_HIDDEN);  
  lv_obj_add_style(lLIndicator, &largeIndicator_style, 0);
  lv_obj_align_to(lLIndicator, Screen1, LV_ALIGN_CENTER, 165, 222);
  lv_label_set_text_fmt(lLIndicator, "%04d", 0);


  // create and rpm guage on main display
  engineRpmGauge = lv_meter_create(Screen1);
  lv_obj_add_style(engineRpmGauge, &largeGuageStyle, 0);

  // declare images for check engine
  LV_IMG_DECLARE(engine_ok); // basically this finds external ref
  LV_IMG_DECLARE(engine_check); // basically this finds external ref
  LV_IMG_DECLARE(oilFault); // basically this finds external ref
  LV_IMG_DECLARE(tempFault); // basically this finds external ref

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

  // Oil Fault Light
  oilFlt = lv_img_create(engineRpmGauge);
  lv_obj_add_flag(oilFlt, LV_OBJ_FLAG_HIDDEN);
  lv_img_set_src(oilFlt, &oilFault);
  lv_obj_align(oilFlt, LV_ALIGN_CENTER, ENGINE_LIGHT_X-80, ENGINE_LIGHT_Y-10);
  lv_obj_set_size(oilFlt, oilFault.header.w, oilFault.header.h);

  // temperature Fault Light
  tempFlt = lv_img_create(engineRpmGauge);
  lv_obj_add_flag(tempFlt, LV_OBJ_FLAG_HIDDEN);
  lv_img_set_src(tempFlt, &tempFault);
  lv_obj_align(tempFlt, LV_ALIGN_CENTER, ENGINE_LIGHT_X+80, ENGINE_LIGHT_Y-25);
  lv_obj_set_size(tempFlt, tempFault.header.w, tempFault.header.h);

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
