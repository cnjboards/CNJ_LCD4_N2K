#include <lvgl.h>
#include "statusBar.h"

lv_style_t screen1_style;
lv_style_t screen2_style;
lv_style_t screen3_style;
lv_style_t screen4_style;
lv_style_t screen5_style;
lv_style_t largeGuageStyle;
lv_style_t smallGuageStyle;
lv_style_t indicator_style;
lv_style_t indicator_style_noBorder;
lv_style_t smallIndicator_style;
lv_style_t smallIndicator_style_noBorder;
lv_style_t largeIndicator_style;
lv_style_t largeIndicator_style_noBorder;
lv_style_t border_style;
lv_style_t popupBox_style;

// setup all lvgl styles
extern "C" void setStyle() {
  
  lv_style_init(&border_style);
  lv_style_set_border_width(&border_style, 2);
  lv_style_set_border_color(&border_style, lv_color_black());

  lv_style_init(&popupBox_style);
  lv_style_set_radius(&popupBox_style, 10);
  lv_style_set_bg_opa(&popupBox_style, LV_OPA_COVER);
  lv_style_set_border_color(&popupBox_style, lv_palette_main(LV_PALETTE_BLUE));
  lv_style_set_border_width(&popupBox_style, 5);

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
  lv_style_set_bg_color(&largeGuageStyle, lv_color_hex(0x3f3636)); //0x2a2424
  lv_style_set_border_color(&largeGuageStyle, lv_color_black());
  lv_style_set_border_width(&largeGuageStyle, 0);
  lv_style_set_bg_opa(&largeGuageStyle, 100);
  lv_style_set_text_color(&largeGuageStyle, lv_color_white());
  lv_style_set_text_font(&largeGuageStyle, &lv_font_montserrat_20);

  // large meter style
  lv_style_init(&smallGuageStyle);
  lv_style_set_bg_color(&smallGuageStyle, lv_color_hex(0x2a2424));
  lv_style_set_border_color(&smallGuageStyle, lv_color_black());
  lv_style_set_bg_opa(&smallGuageStyle, 100);
  lv_style_set_text_color(&smallGuageStyle, lv_color_white());
  lv_style_set_text_font(&smallGuageStyle, &lv_font_montserrat_14);

  // style for indicator readouts
  lv_style_init(&indicator_style);
  lv_style_set_border_width(&indicator_style, 4);
  lv_style_set_radius(&indicator_style,4);
  lv_style_set_border_color(&indicator_style, lv_color_black()/*lv_palette_main(LV_PALETTE_GREY)*/);
  lv_style_set_text_color(&indicator_style, lv_color_white());
  lv_style_set_text_font(&indicator_style, &lv_font_montserrat_20);

  // style for indicator readouts
  lv_style_init(&indicator_style_noBorder);
  lv_style_set_border_width(&indicator_style_noBorder, 0);
  lv_style_set_radius(&indicator_style_noBorder,4);
  lv_style_set_bg_color(&indicator_style_noBorder, lv_color_hex(0x2a2424));
  lv_style_set_bg_opa(&indicator_style_noBorder, 75);
  lv_style_set_text_color(&indicator_style_noBorder, lv_color_white());
  lv_style_set_text_font(&indicator_style_noBorder, &lv_font_montserrat_20);

  // style for indicator readouts
  lv_style_init(&largeIndicator_style);
  lv_style_set_border_color(&largeIndicator_style, lv_color_black()/*lv_palette_main(LV_PALETTE_GREY)*/);
  lv_style_set_border_width(&largeIndicator_style, 4);
  lv_style_set_radius(&largeIndicator_style,12);
  lv_style_set_bg_color(&largeIndicator_style, lv_color_hex(0x808080));
  lv_style_set_bg_opa(&largeIndicator_style, 75);
  lv_style_set_text_color(&largeIndicator_style, lv_color_white());
  lv_style_set_text_font(&largeIndicator_style, &lv_font_montserrat_42);

  // style for indicator readouts
  lv_style_init(&largeIndicator_style_noBorder);
  lv_style_set_border_color(&largeIndicator_style_noBorder, lv_color_hex(0x2a2424));
  lv_style_set_border_width(&largeIndicator_style_noBorder, 0);
  lv_style_set_bg_color(&largeIndicator_style_noBorder, lv_color_hex(0x2a2424));
  lv_style_set_bg_opa(&largeIndicator_style_noBorder, 75);
  lv_style_set_text_color(&largeIndicator_style_noBorder, lv_color_white());
  lv_style_set_text_font(&largeIndicator_style_noBorder, &lv_font_montserrat_42);

  // style for indicator readouts
  lv_style_init(&smallIndicator_style);
  lv_style_set_border_width(&smallIndicator_style, 2);
  lv_style_set_radius(&smallIndicator_style,2);
  lv_style_set_border_color(&smallIndicator_style, lv_color_black());
  lv_style_set_text_font(&indicator_style, &lv_font_montserrat_12);

  // style for indicator readouts
  lv_style_init(&smallIndicator_style_noBorder);
  lv_style_set_border_width(&smallIndicator_style_noBorder, 0);
  lv_style_set_radius(&smallIndicator_style_noBorder,2);
  lv_style_set_text_color(&smallIndicator_style_noBorder, lv_color_white());
  lv_style_set_text_font(&smallIndicator_style_noBorder, &lv_font_montserrat_12);

  // init style elements for status bar
  initStatusStyle();

} // end setstyle
