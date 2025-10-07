// uncomment to dump n2k messages to serial console as plain text
// #define N2K_SERIAL_DUMP

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE /* Enable this to show verbose logging for this file only. */
#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_check.h"
#include <N2kMsg.h>
#include <NMEA2000.h> // https://github.com/ttlappalainen/NMEA2000
#include "sdkconfig.h"
#define ESP32_CAN_TX_PIN (gpio_num_t) CONFIG_ESP32_CAN_TX_PIN  
#define ESP32_CAN_RX_PIN (gpio_num_t) CONFIG_ESP32_CAN_RX_PIN
#include <NMEA2000_esp32xx.h> // https://github.com/jiauka/NMEA2000_esp32xx
#include "N2kMessages.h"
#include <N2KDeviceList.h>
#include "ESP32N2kStream.h"

#define EXAMPLE_N2K_TASK_PRIORITY     10

extern bool startUpDelayDone;
extern uint32_t chipId;

static const char *TAG = "WS_LCD4_N2K_lib";
tNMEA2000_esp32xx NMEA2000;
//tActisenseReader ActisenseReader;

static TaskHandle_t N2K_task_handle = NULL;
ESP32N2kStream Serial;

// List here messages your device will transmit.
const unsigned long TransmitMessages[] PROGMEM={130306L,0};

// Define schedulers for messages. Define schedulers here disabled. Schedulers will be enabled
// on OnN2kOpen so they will be synchronized with system.
// We use own scheduler for each message so that each can have different offset and period.
// Setup periods according PGN definition (see comments on IsDefaultSingleFrameMessage and
// IsDefaultFastPacketMessage) and message first start offsets. Use a bit different offset for
// each message so they will not be sent at same time.
tN2kSyncScheduler WindScheduler(false,1000,500); // Send wind data every 1s

// forward declaration
tN2kDeviceList *pN2kDeviceList;
uint8_t n2kDevicesConnected = 0;

typedef struct {
  unsigned long PGN;
  void (*Handler)(const tN2kMsg &N2kMsg); 
} tNMEA2000Handler;

// globals for n2k values
double locEngRPM = 0;
double locEngOilPres = 0, locEngOilTemp=0, locEngCoolTemp=0, locEngAltVolt=0, locEngFuelRate=0, locEngHours=0, locEngCoolPres=0, locEngFuelPres=0;
double locCOG=0, locSOG=0;
tN2kHeadingReference locRef;
double locWindSpeed, locWindAngle;
tN2kWindReference locWindReference;
unsigned char locBattInst;
double locBattVolt, locBattCurrent, locBatteryTemp;
double locEngBoost;
int8_t locEngTilt;
//double locEngCoolPres, locEngFuelPres;
int8_t locEngLoad, locEngTorque;
double locLevel, locCapacity;
tN2kFluidType locFluidType;
double locTemp, locTempSet;
tN2kTempSource locTempSource;

// forward declaration
// callbacks for items to be displayed
void engineRapidUpdate(const tN2kMsg &);
void engineDynamicUpdate(const tN2kMsg &);
void fluidLevel(const tN2kMsg &);
void batteryStatus(const tN2kMsg &);
void temperatureExtended(const tN2kMsg &);
void cogsogRapid(const tN2kMsg &);

tNMEA2000Handler NMEA2000Handlers[]={
  {127488L,&engineRapidUpdate},
  {127489L,&engineDynamicUpdate},
  {127505L,&fluidLevel},
  {127508L,&batteryStatus},
  {129026, &cogsogRapid},
  {130316L,&temperatureExtended},
  {0,0}
};

// *****************************************************************************
// Call back for NMEA2000 open. This will be called, when library starts bus communication.
// See NMEA2000.SetOnOpen(OnN2kOpen); on setup()
void OnN2kOpen() {
  // Start schedulers now.
  WindScheduler.UpdateNextTime();
}

void HandleStreamN2kMsg(const tN2kMsg &N2kMsg) {
  // N2kMsg.Print(&Serial);
  NMEA2000.SendMsg(N2kMsg);
}

// NMEA 2000 message handler
void HandleNMEA2000Msg(const tN2kMsg &N2kMsg)
{
  int iHandler;
  // Find handler
  for (iHandler=0; NMEA2000Handlers[iHandler].PGN!=0 && !(N2kMsg.PGN==NMEA2000Handlers[iHandler].PGN); iHandler++);
  if (NMEA2000Handlers[iHandler].PGN!=0) {
    NMEA2000Handlers[iHandler].Handler(N2kMsg); 
  } // end if
} // HandleNMEA2000Msg

// *****************************************************************************
double ReadWindAngle() {
  return DegToRad(50); // Read here the measured wind angle e.g. from analog input
}

// *****************************************************************************
double ReadWindSpeed() {
  return 10.3; // Read here the wind speed e.g. from analog input
}

// *****************************************************************************
void SendN2kWind() {
  tN2kMsg N2kMsg;

  if ( WindScheduler.IsTime() ) {
    WindScheduler.UpdateNextTime();
    SetN2kWindSpeed(N2kMsg, 1, ReadWindSpeed(), ReadWindAngle(),N2kWind_Apprent);
    NMEA2000.SendMsg(N2kMsg);
  }
} // end sendN2Kwind

// This is a FreeRTOS task
void N2K_task(void *pvParameters)
{
//  Serial=new ESP32N2kStream();
    ESP_LOGI(TAG, "Starting task");

  NMEA2000.SetProductInformation("V1", // Manufacturer's Model serial code
                                 103, // Manufacturer's product code
                                 "SN00000001",  // Manufacturer's Model ID
                                 "1.0.0.01 (2024-01-15)",  // Manufacturer's Software version code
                                 "WS 4 Inch N2K Display" // Manufacturer's Model version
                                 );
  // Set device information
  NMEA2000.SetDeviceInformation(chipId, // Unique number. Use e.g. Serial number.
                                130, // Device function=Analog -> NMEA2000. See codes on https://web.archive.org/web/20190531120557/https://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                120, // Device class=Sensor Communication Interface. See codes on https://web.archive.org/web/20190531120557/https://www.nmea.org/Assets/20120726%20nmea%202000%20class%20&%20function%20codes%20v%202.00.pdf
                                2041 // Just choosen free from code list on https://web.archive.org/web/20190529161431/http://www.nmea.org/Assets/20121020%20nmea%202000%20registration%20list.pdf
                               );
  // Uncomment 2 rows below to see, what device will send to bus. Use e.g. OpenSkipper or Actisense NMEA Reader                           
  //Serial.begin(115200);
  //NMEA2000.SetForwardStream(&Serial);
  // If you want to use simple ascii monitor like Arduino Serial Monitor, uncomment next line
  //NMEA2000.SetForwardType(tNMEA2000::fwdt_Text); // Show in clear text. Leave uncommented for default Actisense format.

  // If you also want to see all traffic on the bus use N2km_ListenAndNode instead of N2km_NodeOnly below
  NMEA2000.SetMode(tNMEA2000::N2km_ListenAndNode,53);
  // NMEA2000.SetDebugMode(tNMEA2000::dm_Actisense); // Uncomment this, so you can test code without CAN bus chips on Arduino Mega
  #ifdef N2K_SERIAL_DUMP
    NMEA2000.SetForwardStream(&Serial); // PC output on native port
    NMEA2000.SetForwardType(tNMEA2000::fwdt_Text);   // Show in clear text
    NMEA2000.EnableForward(true);
  #endif
  // Here we tell library, which PGNs we transmit
  NMEA2000.ExtendTransmitMessages(TransmitMessages);
  // attach a listener function
  NMEA2000.SetMsgHandler(HandleNMEA2000Msg);
  // Define OnOpen call back. This will be called, when CAN is open and system starts address claiming.
  NMEA2000.SetOnOpen(OnN2kOpen);
  NMEA2000.Open();

#if (configTICK_RATE_HZ != 1000)
#warning "set FreeRTOS tick rate to 1000Hz in menuconfig!"
  ESP_LOGW(TAG, "set FreeRTOS tick rate to 1000Hz in menuconfig!");
#endif
    // main N2K loop
    for (;;)
    {
        // put your main code here, to run repeatedly:
        static TickType_t pxPreviousWakeTime = 0;
        vTaskDelayUntil(&pxPreviousWakeTime, 1); // yield until next tick (should be 1ms) so other tasks can do stuff
        SendN2kWind();
        NMEA2000.ParseMessages();
    } // end for
    vTaskDelete(NULL); // should never get here...
} // end n2k task

// Initialize N2K task
extern "C" int OwnN2KInit(void)
{
    esp_err_t result = ESP_OK;
    ESP_LOGV(TAG, "create task");
    xTaskCreate(
        &N2K_task,            // Pointer to the task entry function.
        "N2K_task",           // A descriptive name for the task for debugging.
        3072,                 // size of the task stack in bytes.
        NULL,                 // Optional pointer to pvParameters
        EXAMPLE_N2K_TASK_PRIORITY, // priority at which the task should run
        &N2K_task_handle      // Optional pass back task handle
    );

    // check for issues on create
    if (N2K_task_handle == NULL)
    {
        ESP_LOGE(TAG, "Unable to create task.");
        result = ESP_ERR_NO_MEM;
        goto err_out;
    } // end if

err_out:
    // clean up
    if (result != ESP_OK)
    {
        if (N2K_task_handle != NULL)
        {
            vTaskDelete(N2K_task_handle);
            N2K_task_handle = NULL;
        } // end if
    } // end if
    return result;
} //end OwnN2KInit

void engineRapidUpdate(const tN2kMsg &N2kMsg) {
    unsigned char SID;
    //ESP_LOGI("N2K", "engineRapidUpdate");
    if (ParseN2kEngineParamRapid(N2kMsg,SID,locEngRPM,locEngBoost,locEngTilt) ) {
      #ifdef SERIALDEBUG
        Serial.print("Engine RPM ");
        Serial.println(locEngRPM);
      #endif  
    } // end if
} // end rapidUpdate

void engineDynamicUpdate(const tN2kMsg &N2kMsg){
    unsigned char SID;
    // from nmea2k def'ns
    /* inline bool ParseN2kEngineDynamicParam(const tN2kMsg &N2kMsg, unsigned char &EngineInstance, double &EngineOilPress,
                      double &EngineOilTemp, double &EngineCoolantTemp, double &AltenatorVoltage,
                      double &FuelRate, double &EngineHours, double &EngineCoolantPress, double &EngineFuelPress,
                      int8_t &EngineLoad, int8_t &EngineTorque) */
    //ESP_LOGI("N2K", "engineDynamicUpdate");
    ParseN2kEngineDynamicParam(N2kMsg, SID, locEngOilPres, locEngOilTemp, locEngCoolTemp, locEngAltVolt, locEngFuelRate, locEngHours, locEngCoolPres, locEngFuelPres, locEngLoad, locEngTorque);
} // end enfineDynamicUpdate

void fluidLevel(const tN2kMsg &N2kMsg){
    unsigned char SID;
    if (ParseN2kFluidLevel(N2kMsg, SID, locFluidType, locLevel, locCapacity) ) {
      if (locFluidType == N2kft_Fuel) {
      #ifdef SERIALDEBUG
        Serial.print("Engine Fuel Level ");
        Serial.print(locLevel);
        Serial.println(" percent");
      #endif
      } // end if
    } // end if
} // end fluidLevel

void batteryStatus(const tN2kMsg &N2kMsg){
    unsigned char SID;
    /*inline bool ParseN2kDCBatStatus(const tN2kMsg &N2kMsg, unsigned char &BatteryInstance, double &BatteryVoltage, double &BatteryCurrent,
                     double &BatteryTemperature, unsigned char &SID)*/

    if (ParseN2kDCBatStatus(N2kMsg, locBattInst, locBattVolt, locBattCurrent, locBatteryTemp, SID) ) {
      if (locBattInst == 0) {
      #ifdef SERIALDEBUG        
        Serial.print("House Batt Voltage ");
        Serial.print(locBattVolt);
        Serial.println(" volts");
      #endif
      } // end if
    } // end if
} // end batteryStatus

void temperatureExtended(const tN2kMsg &N2kMsg){
    unsigned char SID, locTempInstance;
    /*bool ParseN2kPGN130316(const tN2kMsg &N2kMsg, unsigned char &SID, unsigned char &TempInstance, tN2kTempSource &TempSource,
                     double &ActualTemperature, double &SetTemperature) */

    if (ParseN2kPGN130316(N2kMsg, SID, locTempInstance, locTempSource, locTemp, locTempSet) ) {
      if (locTempSource == N2kts_EngineRoomTemperature) {
      #ifdef SERIALDEBUG
        Serial.print("Engine Room Temp ");
        Serial.print(locTemp);
        Serial.println(" Kelvin");
      #endif
      } // end if
      if (locTempSource == N2kts_ExhaustGasTemperature) {
      #ifdef SERIALDEBUG
        Serial.print("Engine Exhaust Gas Temp ");
        Serial.print(locTemp);
        Serial.println(" Kelvin");
      #endif
      } // end if
    } // end if
} // end temperatureExtended

void cogsogRapid(const tN2kMsg &N2kMsg){
    unsigned char SID, locTempInstance;

    /*inline bool ParseN2kCOGSOGRapid(const tN2kMsg &N2kMsg, unsigned char &SID, tN2kHeadingReference &ref, double &COG, double &SOG) {
        return ParseN2kPGN129026(N2kMsg,SID,ref,COG,SOG); */

    if (ParseN2kCOGSOGRapid(N2kMsg, SID, locRef, locCOG, locSOG) ) {
      #ifdef SERIALDEBUG
        Serial.print("COG ");
        Serial.print(locCOG);
        Serial.print("   SOG ");
        Serial.print(locSOG);
        Serial.println(" m/s");
      #endif          
    } // end if
} // end temperatureExtended
