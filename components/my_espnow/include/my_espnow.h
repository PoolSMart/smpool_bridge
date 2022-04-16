/* ESPNOW Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#ifndef ESPNOW_EXAMPLE_H
#define ESPNOW_EXAMPLE_H

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_now.h"
#include "esp_crc.h"
#include "my_mqtt.h"

#ifdef __cplusplus
extern "C" {
#endif

extern xQueueHandle s_espnow_queue;

typedef enum {
  BRIDGE,
  THERMOMETER,
  LUXMETER,
  SWITCH,
  TOUCH,
  TOUCH_RESPONSE
} smpool_device_type;

//data to be send to bridge
typedef struct {
    u_int32_t pressure;                 //Measured pressure value (Pa).
    u_int16_t flow;                     //Measured float value of (dm^3 / 3600s).
    u_int8_t switch_status;             //Boolean which represents if switch is ON/OFF.
} switch_data_t;

//data from luxmeter
typedef struct {
    float lux;                          //Measured value of light intensity.
    u_int8_t bat_percentage;            //Percentage of remaining battery capacity.
} lux_data_t;

//data from thermometer
typedef struct {
    float temperature;                  //Measured value temperature.
    float ph;                           //Measured value of watter ph.
    u_int8_t bat_percentage;            //Percentage of remaining battery capacity.
} therm_data_t;

//data to be recieved from bridge
typedef u_int8_t switch_request_t;        //Boolean which represents if switch should be ON. ON-true, OFF-false.

//Parameters of sending ESPNOW data.
typedef struct {
    smpool_device_type dev_type;          //Indicate type of device for bridge
    uint16_t delay;                       //Delay between sending two ESPNOW data, unit: ms.
    int buf_len;                          //Length of ESPNOW data to be sent, unit: byte.
    uint8_t *buffer;                      //Buffer pointing to data to be sent. 
    uint8_t dest_mac[ESP_NOW_ETH_ALEN];   //MAC address of destination device.
    int data_len;
    uint8_t * data_to_send;               //Data will stay untuched.
} send_param_t;

//whole sending message
typedef struct {
    uint8_t dev_type;                     //Broadcast or unicast ESPNOW data.
    uint16_t crc;                         //CRC16 value of ESPNOW data.
    uint8_t payload_len;
    uint8_t payload[0];                   //Real payload of ESPNOW data.
} __attribute__((packed)) espnow_data_t;



//functions
void wifi_init(void);
int espnow_data_parse(uint8_t *data, uint16_t data_len, uint8_t *recv_dev_type);
void espnow_data_prepare(send_param_t *send_param);
void get_message(uint8_t *data, void ** message);
esp_err_t myEspNowSetup(send_param_t** send_param, int data_len, smpool_device_type dev_type);
void myBridgeEspNowTask(void* send_param_pv);
void my_send(send_param_t* send_param_p);
void my_espnow_deinit(send_param_t *send_param);




/* ESPNOW can work in both station and softap mode. It is configured in menuconfig. */
#if CONFIG_ESPNOW_WIFI_MODE_STATION
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_AP
#endif

#define ESPNOW_QUEUE_SIZE           8

#define IS_BROADCAST_ADDR(addr) (memcmp(addr, s_broadcast_mac, ESP_NOW_ETH_ALEN) == 0)



typedef enum {
    ESPNOW_SEND_CB,
    ESPNOW_RECV_CB,
} espnow_event_id_t;

typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    esp_now_send_status_t status;
} espnow_event_send_cb_t;

typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    uint8_t *data;
    int data_len;
} espnow_event_recv_cb_t;

typedef union {
    espnow_event_send_cb_t send_cb;
    espnow_event_recv_cb_t recv_cb;
} espnow_event_info_t;

/* When ESPNOW sending or receiving callback function is called, post event to ESPNOW task. */
typedef struct {
    espnow_event_id_t id;
    espnow_event_info_t info;
} espnow_event_t;

enum {
    ESPNOW_DATA_BROADCAST,
    ESPNOW_DATA_UNICAST,
    ESPNOW_DATA_MAX,
};





























// /* ESPNOW Example

//    This example code is in the Public Domain (or CC0 licensed, at your option.)

//    Unless required by applicable law or agreed to in writing, this
//    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
//    CONDITIONS OF ANY KIND, either express or implied.
// */

// #include <stdlib.h>
// #include <time.h>
// #include <string.h>
// #include <assert.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/semphr.h"
// #include "freertos/timers.h"
// #include "nvs_flash.h"
// #include "esp_event.h"
// #include "esp_netif.h"
// #include "esp_wifi.h"
// #include "esp_log.h"
// #include "esp_system.h"
// #include "esp_now.h"
// #include "esp_crc.h"


// #ifndef ESPNOW_EXAMPLE_H
// #define ESPNOW_EXAMPLE_H


// typedef enum {
//   BRIDGE,
//   THERMOMETER,
//   LUXMETER,
//   SWITCH
// } smpool_device_type;

// //data to be send to bridge
// typedef struct {
//     u_int32_t pressure;                 //Measured pressure value (Pa).
//     u_int16_t flow;                     //Measured float value of (dm^3).
//     u_int8_t switch_status;             //Boolean which represents if switch is ON/OFF.
// } switch_data_t;

// //data from luxmeter
// typedef struct {
//     float lux;                          //Measured value of light intensity.
//     u_int8_t bat_percentage;            //Percentage of remaining battery capacity.
// } lux_data_t;

// //data from thermometer
// typedef struct {
//     float temperature;                  //Measured value temperature.
//     float ph;                           //Measured value of watter ph.
//     u_int8_t bat_percentage;            //Percentage of remaining battery capacity.
// } therm_data_t;

// //Parameters of sending ESPNOW data.
// typedef struct {
//     smpool_device_type dev_type;          //Indicate type of device for bridge
//     uint16_t delay;                       //Delay between sending two ESPNOW data, unit: ms.
//     int buf_len;                          //Length of ESPNOW data to be sent, unit: byte.
//     uint8_t *buffer;                      //Buffer pointing to data to be sent. 
//     uint8_t dest_mac[ESP_NOW_ETH_ALEN];   //MAC address of destination device.
//     int data_len;
//     uint8_t * data_to_send;               //Data will stay untuched.
// } send_param_t;

// //whole sending message
// typedef struct {
//     uint8_t dev_type;                     //Broadcast or unicast ESPNOW data.
//     uint16_t crc;                         //CRC16 value of ESPNOW data.
//     uint8_t payload_len;
//     uint8_t payload[0];                   //Real payload of ESPNOW data.
// } __attribute__((packed)) espnow_data_t;



// //functions
// void example_wifi_init(void);
// esp_err_t sendData(lux_data_t * data);
// void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
// void example_espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len);
// void espnow_bridge_task(void *pvParameter);
// void espnow_data_prepare(send_param_t *send_param);
// int espnow_data_parse(uint8_t *data, uint16_t data_len, uint8_t *recv_dev_type);


// xQueueHandle s_espnow_queue;


// /* ESPNOW can work in both station and softap mode. It is configured in menuconfig. */
// #if CONFIG_ESPNOW_WIFI_MODE_STATION
// #define ESPNOW_WIFI_MODE WIFI_MODE_STA
// #define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
// #else
// #define ESPNOW_WIFI_MODE WIFI_MODE_AP
// #define ESPNOW_WIFI_IF   ESP_IF_WIFI_AP
// #endif

// #define ESPNOW_QUEUE_SIZE           6

// #define IS_BROADCAST_ADDR(addr) (memcmp(addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN) == 0)

// static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };


// typedef enum {
//     ESPNOW_SEND_CB,
//     ESPNOW_RECV_CB,
// } espnow_event_id_t;

// typedef struct {
//     uint8_t mac_addr[ESP_NOW_ETH_ALEN];
//     esp_now_send_status_t status;
// } espnow_event_send_cb_t;

// typedef struct {
//     uint8_t mac_addr[ESP_NOW_ETH_ALEN];
//     uint8_t *data;
//     int data_len;
// } espnow_event_recv_cb_t;

// typedef union {
//     espnow_event_send_cb_t send_cb;
//     espnow_event_recv_cb_t recv_cb;
// } espnow_event_info_t;

// /* When ESPNOW sending or receiving callback function is called, post event to ESPNOW task. */
// typedef struct {
//     espnow_event_id_t id;
//     espnow_event_info_t info;
// } espnow_event_t;

// enum {
//     ESPNOW_DATA_BROADCAST,
//     ESPNOW_DATA_UNICAST,
//     ESPNOW_DATA_MAX,
// };


// #endif


#ifdef __cplusplus
}
#endif

#endif