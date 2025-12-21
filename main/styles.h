#pragma once
#include <lvgl.h>

// lvgl styles
extern lv_style_t screen1_style;
extern lv_style_t screen2_style;
extern lv_style_t screen3_style;
extern lv_style_t screen4_style;
extern lv_style_t screen5_style;
extern lv_style_t largeGuageStyle;
extern lv_style_t smallGuageStyle;
extern lv_style_t indicator_style;
extern lv_style_t indicator_style_noBorder;
extern lv_style_t smallIndicator_style;
extern lv_style_t smallIndicator_style_noBorder;
extern lv_style_t largeIndicator_style;
extern lv_style_t largeIndicator_style_noBorder;

#ifdef __cplusplus
extern "C" {
#endif

    void setStyle();

#ifdef __cplusplus
}
#endif
