#pragma once
#include "freertos/event_groups.h"
#include <N2KTypes.h>
#include "esp_io_expander_tca9554.h"
#include "esp_lcd_panel_ops.h"

extern esp_io_expander_handle_t io_expander;
extern esp_lcd_panel_handle_t lcd_handle;

extern int counter;
extern int State;
extern int old_State;
extern int move_flag;
extern int button_flag;
extern int flesh_flag;
extern int shortButtonStateLatched;
extern int longButtonStateLatched;

extern double locEngRPM;
extern double locEngOilPres;
extern double locEngOilTemp;
extern double locEngCoolTemp; 
extern double locEngAltVolt; 
extern double locEngFuelRate; 
extern double locEngHours; 
extern double locEngCoolPres;
extern double locEngFuelPres;

extern double locCOG;
extern double locSOG;
extern tN2kHeadingReference locRef;
extern double locWindSpeed, locWindAngle;
extern double locWindSpeedApp, locWindAngleApp;
extern double locWindSpeedTrue, locWindAngleTrue;
extern tN2kWindReference locWindReference;
extern tN2kEngineDiscreteStatus1 locStat1; 
extern tN2kEngineDiscreteStatus2 locStat2;
extern uint8_t n2kConnected;
extern bool n2kUp;

extern char myIpAddressSta[];
extern char *wifiStaSsid;
extern EventGroupHandle_t s_wifi_event_group;
