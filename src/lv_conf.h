#pragma once

#define LV_COLOR_DEPTH 1 /* 单色屏必须为 1 */
#define LV_HOR_RES_MAX 128
#define LV_VER_RES_MAX 64
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "esp_timer.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (esp_timer_get_time() / 1000LL)

#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0
#define LV_USE_LOG 0