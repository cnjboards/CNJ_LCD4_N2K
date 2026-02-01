#pragma once

// for now decide where data comes form, eventually done under program control
// #define USEN2KDATA

#include "freertos/event_groups.h"
#include <N2KTypes.h>
#include "esp_io_expander_tca9554.h"
#include "esp_lcd_panel_ops.h"

extern esp_io_expander_handle_t io_expander;
extern esp_lcd_panel_handle_t lcd_handle;

// n2k data values
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
extern double locSTW;
extern tN2kHeadingReference locRef;
extern double locWindSpeed, locWindAngle;
extern double locWindSpeedApp, locWindAngleApp;
extern double locWindSpeedTrue, locWindAngleTrue;
extern tN2kWindReference locWindReference;
extern double locDepthBelowKeel;
extern tN2kEngineDiscreteStatus1 locStat1; 
extern tN2kEngineDiscreteStatus2 locStat2;

// some globals for mqtt values
extern double mqttEngRPM;
extern double mqttEngOilPres;
extern double mqttEngOilTemp; 
extern double mqttEngCoolTemp;
extern double mqttEngAltVolt;
extern double mqttCOG, mqttSOG, mqttSTW;
extern double mqttWindSpeedApp, mqttWindAngleApp;
extern double mqttWindSpeedTrue, mqttWindAngleTrue;
extern double mqttDepthBelowKeel;
extern double mqttBattVolt, mqttBattCurrent, mqttBatteryTemp;
extern double mqttLevel, mqttCapacity;
extern bool mqttCheckEngineAlarm;
extern bool mqttLowOilPressureAlarm;
extern bool mqttOverTemperatureAlarm;

// n2k link status
extern uint8_t n2kConnected;
extern bool n2kUp;

// wifi stuff
extern char myIpAddressSta[];
extern char *wifiStaSsid;
extern EventGroupHandle_t s_wifi_event_group;

// misc
// global flag for startup complete
extern bool startUpDelayDone;
extern uint32_t chipId;
extern bool flashBit;

// used for wifi sync
extern bool myWifiStaConnected;
extern bool myWifiStaIpValid;

// used for rads to deg conversion
#define PI 3.14159265358979323846
