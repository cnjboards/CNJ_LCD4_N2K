#pragma once

#include "lvgl.h"

extern lv_obj_t *statusBar;

#ifdef __cplusplus
extern "C" {
#endif

    void initStatusStyle();
    void buildStatusBar(lv_obj_t *);
    void updateStatusBar(lv_timer_t *);
    
#ifdef __cplusplus
}
#endif
