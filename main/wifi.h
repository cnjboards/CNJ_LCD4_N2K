#pragma once
#include "esp_log.h"

extern void wifi_init_sta(void);
extern void wifi_init_ap_sta(void);
extern esp_err_t http_server_init(void);
