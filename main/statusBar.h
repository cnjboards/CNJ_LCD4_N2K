#pragma once

#include "lvgl.h"

extern "C" void initStatusStyle();
extern "C" void buildStatusBar(lv_obj_t *);
extern "C" void updateStatusBar(lv_timer_t *);