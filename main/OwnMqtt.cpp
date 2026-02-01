// uncomment to dump n2k messages to serial console as plain text
// #define N2K_SERIAL_DUMP

#define LOG_LOCAL_LEVEL ESP_LOG_WARN

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_check.h"
#include "sdkconfig.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "mqtt_client.h"
#include "globals.h"

static const char *TAG = "CNJ_LCD4_MQTT_LIB";

// hardcode mqtt credentials for now
#define MQTT_HOST "mqtt://192.168.2.135"
#define MQTT_UNAME "mqttTest"
#define MQTT_PASSWD "1Qwerty!"

// used for subscriptions 
#define MQTTWILDCARD "#"

// used to prepend to subscriptions
char globalSub[] = "vessels/self/";

// list of mqtt topics to subscribe to
// add here for more data points
// also need to add handler code in parseMqttData
// TBD future maybe read a list from uSD or flash and populate a dynamic data structure
static char* mqttSubList[] = {
    "propulsion/port/revolutions",
    "propulsion/port/oilPressure",
    "propulsion/port/alternatorVoltage",
    "propulsion/port/temperature",
    "environment/depth/belowSurface",
    "navigation/speedThroughWater",
    "navigation/speedOverGround",
    "environment/wind/speedOverGround",
    "environment/wind/angleApparent",
    "environment/wind/speedApparent",
    "environment/wind/directionTrue",
    "notifications/propulsion/port/checkEngine",
    "notifications/propulsion/port/overTemperature",
    "notifications/propulsion/port/lowOilPressure",
    0
};

// task stuff
#define MQTT_TASK_PRIORITY     20
static TaskHandle_t MQTT_task_handle = NULL;

// forward declares
static void mqtt_start();
static void mqtt_event_handler(void *, esp_event_base_t, int32_t, void *);
void parseMqttData(esp_mqtt_event_handle_t );
void subscribeMqttData(esp_mqtt_client_handle_t );

// some globals for mqtt data values
double mqttEngRPM = 0;
double mqttEngOilPres = 0;
double mqttEngOilTemp=0; 
double mqttEngCoolTemp=0;
double mqttEngAltVolt=0;
double mqttCOG=0, mqttSOG=0, mqttSTW=0;
double mqttWindSpeedApp=0, mqttWindAngleApp=0;
double mqttWindSpeedTrue=0, mqttWindAngleTrue=0;
double mqttDepthBelowKeel=0;
double mqttBattVolt=0, mqttBattCurrent=0, mqttBatteryTemp=0;
double mqttLevel=0, mqttCapacity=0;
bool mqttCheckEngineAlarm=0;
bool mqttLowOilPressureAlarm=0;
bool mqttOverTemperatureAlarm=0;

// Task to handle mqtt comms
void MQTT_task(void *pvParameters)
{
  static TickType_t pxPreviousWakeTime;

  ESP_LOGI(TAG, "Mqtt task Start");

  // delay a bit
  vTaskDelay(pdMS_TO_TICKS(4000));

  // make sure wifi is connected and ip assigned before trying mqtt
  if ((myWifiStaConnected & myWifiStaIpValid)) {
    // start mqtt
    ESP_LOGV(TAG, "Mqtt - wifi connected, starting mqtt client");
    mqtt_start();
  } // end while

  // used for controlling rate of periodic polling of mqtt
  pxPreviousWakeTime = xTaskGetTickCount();
  // main Mqtt processing loop
  for (;;)
  {
    // reset previous wake time
    vTaskDelayUntil(&pxPreviousWakeTime, 100); // yield for a while, if fast response then use 1 so other tasks can do stuff

  } // end for
  vTaskDelete(NULL); // should never get here...
} // end n2k task

// Initialize Mqtt task
extern "C" int OwnMqttInit(void)
{
  // enable verbose logging in this module
  esp_log_level_set(TAG, ESP_LOG_VERBOSE);

  esp_err_t result = ESP_OK;
    ESP_LOGV(TAG, "Create Mqtt task");
    xTaskCreate(
        &MQTT_task,            // Pointer to the task entry function.
        "Mqtt_task",           // A descriptive name for the task for debugging.
        8192,                 // size of the task stack in bytes.
        NULL,                 // Optional pointer to pvParameters
        MQTT_TASK_PRIORITY, // priority at which the task should run
        &MQTT_task_handle      // Optional pass back task handle
    );

    // check for issues on create
    if (MQTT_task_handle == NULL)
    {
        ESP_LOGE(TAG, "Unable to create N2K task.");
        result = ESP_ERR_NO_MEM;
        goto err_out;
    } // end if

err_out:
    // clean up
    if (result != ESP_OK)
    {
        if (MQTT_task_handle != NULL)
        {
            vTaskDelete(MQTT_task_handle);
            MQTT_task_handle = NULL;
        } // end if
    } // end if
    return result;
} //end OwnMqttInit

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t )event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");        
        // subscribe to all the data
        subscribeMqttData(client);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED - Initiated reboot in 2 seconds");
        // delay a bit
        vTaskDelay(pdMS_TO_TICKS(2000));
        // tbd - need to handle. For now just reboot
        esp_sleep_enable_timer_wakeup(1);
        esp_deep_sleep_start();    
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d, return code=0x%02x ", event->msg_id, (uint8_t)*event->data);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        // tbd need handler function for all subscribed data
        parseMqttData(event);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGE(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        } // end if
        break;
    default:
        ESP_LOGE(TAG, "Other mqtt event id:%d", event->event_id);
        break;
    }
} // end mqtt_event_handler

// start mqtt client
static void mqtt_start(void)
{
  esp_mqtt_client_config_t mqtt_cfg = {0};

  // mqtt params.
  //mqtt_cfg.broker.address.uri = (char *)"mqtt://192.168.2.135";
  mqtt_cfg.broker.address.uri = MQTT_HOST;
  mqtt_cfg.broker.address.port = 1883;
  mqtt_cfg.credentials.username = MQTT_UNAME;
  mqtt_cfg.credentials.authentication.password = MQTT_PASSWD;

  esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
  /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
  esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event_handler, NULL); 
  esp_mqtt_client_start(client);
} // end mqtt_start

// loop thru a list to issue subscriptions
void subscribeMqttData(esp_mqtt_client_handle_t client){
    int msg_id;
    char subBuf[256];
    // local pointer point at static list
    char **locmqttSubList=mqttSubList;

    // subscribe to all valus in list
    while(*locmqttSubList) {
        // build subscription string from list
        // append base
        strcpy(subBuf, globalSub);
        // append subscription string and advance to next string
        strcat(subBuf, *locmqttSubList++);
        // attempt subscription
        msg_id = esp_mqtt_client_subscribe(client, subBuf, 0);
        if (msg_id)
            // success
            ESP_LOGI(TAG, "sent subscribe $%s , msg_id=%d", subBuf, msg_id);
        else
            // error
            ESP_LOGW(TAG, "Error with subscription sent subscribe $%s", subBuf);
    } // end while
} // end subscribeMqttData


// here we parse all the messages and assign to global vars
// kind of brute force for now....
void parseMqttData(esp_mqtt_event_handle_t event)
{
    // need to null terminate the ebent data string since some cases are missing null termination
    event->data[event->data_len]='\0'; 

    // brute force for now
    if (strstr(event->topic, "propulsion/port/revolutions")) {
        ESP_LOGI(TAG,"Assigned TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG,"DATA=%.*s", event->data_len, event->data);              
        mqttEngRPM = strtod(event->data, 0) * 60.0; // convert and turn into rpm from hz
    } else if (strstr(event->topic, "propulsion/port/oilPressure")) {
        ESP_LOGI(TAG,"Assigned TOPIC=%.*s", event->topic_len, event->topic);  
        ESP_LOGI(TAG,"DATA=%.*s", event->data_len, event->data);  
        mqttEngOilPres = strtod(event->data, 0);
    } else if (strstr(event->topic, "propulsion/port/alternatorVoltage")) {
        ESP_LOGI(TAG,"Assigned TOPIC=%.*s", event->topic_len, event->topic);    
        ESP_LOGI(TAG,"DATA=%.*s", event->data_len, event->data);  
        mqttEngAltVolt = strtod(event->data, 0);
    } else if (strstr(event->topic, "propulsion/port/temperature")) {
        ESP_LOGI(TAG,"Assigned TOPIC=%.*s", event->topic_len, event->topic);    
        ESP_LOGI(TAG,"DATA=%.*s", event->data_len, event->data);  
        mqttEngCoolTemp = strtod(event->data, 0);
    } else if (strstr(event->topic, "environment/depth/belowSurface")) {
        ESP_LOGI(TAG,"Assigned TOPIC=%.*s", event->topic_len, event->topic);    
        ESP_LOGI(TAG,"DATA=%.*s", event->data_len, event->data);  
        mqttDepthBelowKeel = strtod(event->data, 0);
    } else if (strstr(event->topic, "navigation/speedOverGround")) {
        ESP_LOGI(TAG,"Assigned TOPIC=%.*s", event->topic_len, event->topic);    
        ESP_LOGI(TAG,"DATA=%.*s", event->data_len, event->data);  
        mqttSOG = strtod(event->data, 0);
    } else if (strstr(event->topic, "speedThroughWater") && !strstr(event->topic, "speedThroughWaterReferenceType")) {
        ESP_LOGI(TAG,"Assigned TOPIC=%.*s", event->topic_len, event->topic);    
        ESP_LOGI(TAG,"DATA=%.*s", event->data_len, event->data);  
        mqttSTW = strtod(event->data, 0);
    } else if (strstr(event->topic, "environment/wind/speedOverGround")) {
        ESP_LOGI(TAG,"Assigned TOPIC=%.*s", event->topic_len, event->topic);    
        ESP_LOGI(TAG,"DATA=%.*s", event->data_len, event->data);  
        mqttWindSpeedTrue = strtod(event->data, 0);
    } else if (strstr(event->topic, "environment/wind/directionTrue")) {
        ESP_LOGI(TAG,"Assigned TOPIC=%.*s", event->topic_len, event->topic);    
        ESP_LOGI(TAG,"DATA=%.*s", event->data_len, event->data);  
        mqttWindAngleTrue = strtod(event->data, 0);
    } else if (strstr(event->topic, "environment/wind/speedApparent")) {
        ESP_LOGI(TAG,"Assigned TOPIC=%.*s", event->topic_len, event->topic);    
        ESP_LOGI(TAG,"DATA=%.*s", event->data_len, event->data);  
        mqttWindSpeedApp = strtod(event->data, 0);
    } else if (strstr(event->topic, "environment/wind/angleApparent")) {
        ESP_LOGI(TAG,"Assigned TOPIC=%.*s", event->topic_len, event->topic);    
        ESP_LOGI(TAG,"DATA=%.*s", event->data_len, event->data);  
        mqttWindAngleApp = strtod(event->data, 0);
    } else if (strstr(event->topic, "notifications/propulsion/port/checkEngine")) {
        ESP_LOGI(TAG,"Assigned TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG,"DATA=%.*s", event->data_len, event->data);  
        if (strstr(event->data, "alarm")){
            mqttCheckEngineAlarm = true;
        } else {
            mqttCheckEngineAlarm = false;
        } // end if
    } else if (strstr(event->topic, "notifications/propulsion/port/overTemperature")) {
        ESP_LOGI(TAG,"Assigned TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG,"DATA=%.*s", event->data_len, event->data);  
        if (strstr(event->data, "alarm")){
            mqttOverTemperatureAlarm = true;
        } else {
            mqttOverTemperatureAlarm = false;
        } // end if
    } else if (strstr(event->topic, "notifications/propulsion/port/lowOilPressure")) {
        ESP_LOGI(TAG,"Assigned TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG,"DATA=%.*s", event->data_len, event->data);  
        if (strstr(event->data, "alarm")){
            mqttLowOilPressureAlarm = true;
        } else {
            mqttLowOilPressureAlarm = false;
        } // end if
    } else {
        // unassigned so send warning log, we should not subscribe to anything we dont need
        ESP_LOGW(TAG,"Unassigned TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGW(TAG,"DATA=%.*s", event->data_len, event->data);  
    } // end if
} // end parseMqttData


