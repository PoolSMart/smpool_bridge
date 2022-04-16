/* ESPNOW Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
   This example shows how to use ESPNOW.
   Prepare two device, one for sending ESPNOW data and another for receiving
   ESPNOW data.
*/

#include "my_espnow.h"

#define ESPNOW_MAXDELAY 512

static const char *TAG = "ESPNOW";


xQueueHandle s_espnow_queue;


static uint16_t s_espnow_seq[ESPNOW_DATA_MAX] = { 0, 0 };

void my_espnow_deinit(send_param_t *send_param);

SemaphoreHandle_t send_semaphore = NULL;

typedef enum{
  OFF,
  ON
} switch_state_t;
extern switch_state_t switch_state;

static uint8_t broadcast_mac_s[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };


/* WiFi should start before using ESPNOW */
void wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(ESPNOW_WIFI_MODE) );
    ESP_ERROR_CHECK( esp_wifi_start());
#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK( esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
#endif
}



void my_espnow_deinit(send_param_t *send_param)
{
    free(send_param->buffer);
    free(send_param);
    vSemaphoreDelete(s_espnow_queue);
    vSemaphoreDelete(send_semaphore);
    esp_now_deinit();
}



/* ESPNOW sending or receiving callback function is called in WiFi task.
 * Users should not do lengthy operations from this task. Instead, post
 * necessary data to a queue and handle it from a lower priority task. */
static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    espnow_event_t evt;
    espnow_event_send_cb_t *send_cb = &evt.info.send_cb;

    if (mac_addr == NULL) {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    evt.id = ESPNOW_SEND_CB;
    memcpy(send_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    send_cb->status = status;
    if (xQueueSend(s_espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE) {
        ESP_LOGW(TAG, "Send send queue fail");
    }
}


void espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    espnow_event_t evt;
    espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;

    if (mac_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    evt.id =ESPNOW_RECV_CB;
    memcpy(recv_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    recv_cb->data = malloc(len);
    if (recv_cb->data == NULL) {
        ESP_LOGE(TAG, "Malloc receive data fail");
        return;
    }
    memcpy(recv_cb->data, data, len);
    recv_cb->data_len = len;
    if (xQueueSend(s_espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE) {
        ESP_LOGW(TAG, "Send receive queue fail");
        free(recv_cb->data);
    }
}



/* Prepare ESPNOW data to be sent. */
void espnow_data_prepare(send_param_t *send_param)
{
    espnow_data_t *buf = (espnow_data_t *)send_param->buffer;

    assert(send_param->buf_len >= sizeof(espnow_data_t));

    buf->dev_type = send_param->dev_type;
    buf->payload_len = send_param->data_len;
    buf->crc = 0;
    memcpy(buf->payload, send_param->data_to_send, send_param->data_len);
    buf->crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, send_param->buf_len);

    //todo na zkalade typu by mohlo switchovat jak velka je payload len
}

void get_message(uint8_t *data, void ** message){
    espnow_data_t *buf = (espnow_data_t *)data;
    memcpy(*message, buf->payload, buf->payload_len);
    //todo  free?
}


/* Parse received ESPNOW data. */
int espnow_data_parse(uint8_t *data, uint16_t data_len, uint8_t *recv_dev_type)
{
    espnow_data_t *buf = (espnow_data_t *)data;
    uint16_t crc, crc_cal = 0;

    if (data_len < sizeof(espnow_data_t)) {
        ESP_LOGE(TAG, "Receive ESPNOW data too short, len:%d", data_len);
        return -1;
    }
    *recv_dev_type = buf->dev_type;
    crc = buf->crc; 
    buf->crc = 0;
    crc_cal = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, data_len);

    if (crc_cal != crc) {
        ESP_LOGE(TAG, "CRC does not matchd");
        return -1;
    }
    return  1;
}


void my_send(send_param_t* send_param_p){
    if(send_semaphore != NULL){
      if(xSemaphoreTake(send_semaphore, portMAX_DELAY) == pdTRUE){
        espnow_data_prepare(send_param_p);
        if (esp_now_send(send_param_p->dest_mac, send_param_p->buffer, send_param_p->buf_len) != ESP_OK) {
          ESP_LOGE("ESPNOW", "Send error");
        }
      }
      else{
          return;
      }
      xSemaphoreGive(send_semaphore);
    }
}

void my_send_task(void* pv_send_param){
    send_param_t* send_param_p = (send_param_t*) pv_send_param;
    vTaskDelay(pdMS_TO_TICKS(3000));
    my_send(send_param_p);
    vTaskDelete(NULL);
}


esp_err_t myEspNowSetup(send_param_t** send_param, int data_len, smpool_device_type dev_type){
    send_param_t *send_param_p;
    vSemaphoreCreateBinary( send_semaphore );
    s_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(espnow_event_t));
    if (s_espnow_queue == NULL) {
        ESP_LOGE(TAG, "Create mutex fail");
        return ESP_FAIL;
    }
    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(espnow_recv_cb) );
    /* Set primary master key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );
    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t *broadcast_peer = malloc(sizeof(esp_now_peer_info_t));
    if (broadcast_peer == NULL) {
        ESP_LOGE(TAG, "Malloc peer information fail");
        vSemaphoreDelete(s_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(broadcast_peer, 0, sizeof(esp_now_peer_info_t));
    broadcast_peer->channel = 0;
    broadcast_peer->ifidx = ESPNOW_WIFI_IF;
    broadcast_peer->encrypt = false;
    memcpy(broadcast_peer->peer_addr, broadcast_mac_s, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(broadcast_peer) );
    free(broadcast_peer);
    /* Initialize sending parameters. */
    *send_param = malloc(sizeof(send_param_t));
    send_param_p = *send_param;
    memset(send_param_p, 0, sizeof(send_param_t));
    if (send_param == NULL) {
        ESP_LOGE(TAG, "Malloc send parameter fail");
        vSemaphoreDelete(s_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    send_param_p->dev_type = dev_type;
    send_param_p->delay = CONFIG_ESPNOW_SEND_DELAY;
    send_param_p->buf_len = sizeof(espnow_data_t) + data_len ;
    send_param_p->buffer = malloc(sizeof(espnow_data_t) + data_len );
    if (send_param_p->buffer == NULL) {
        ESP_LOGE(TAG, "Malloc send buffer fail");
        free(send_param);
        vSemaphoreDelete(s_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memcpy(send_param_p->dest_mac, broadcast_mac_s, ESP_NOW_ETH_ALEN);
    vTaskDelay(pdMS_TO_TICKS(1000));
    return ESP_OK;
}
















// /* ESPNOW Example

//    This example code is in the Public Domain (or CC0 licensed, at your option.)

//    Unless required by applicable law or agreed to in writing, this
//    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
//    CONDITIONS OF ANY KIND, either express or implied.
// */

// /*
//    This example shows how to use ESPNOW.
//    Prepare two device, one for sending ESPNOW data and another for receiving
//    ESPNOW data.
// */

// #include "my_espnow.h"

// #define ESPNOW_MAXDELAY 512

// static const char *TAG = "ESPNOW";




// static uint16_t s_example_espnow_seq[EXAMPLE_ESPNOW_DATA_MAX] = { 0, 0 };

// static void example_espnow_deinit(send_param_t *send_param);

// /* WiFi should start before using ESPNOW */
// void example_wifi_init(void)
// {
//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());
//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
//     ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
//     ESP_ERROR_CHECK( esp_wifi_set_mode(ESPNOW_WIFI_MODE) );
//     ESP_ERROR_CHECK( esp_wifi_start());
// #if CONFIG_ESPNOW_ENABLE_LONG_RANGE
//     ESP_ERROR_CHECK( esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
// #endif
// }

// static void example_espnow_deinit(send_param_t *send_param)
// {
//     free(send_param->buffer);
//     free(send_param);
//     vSemaphoreDelete(s_example_espnow_queue);
//     esp_now_deinit();
// }

// /* ESPNOW sending or receiving callback function is called in WiFi task.
//  * Users should not do lengthy operations from this task. Instead, post
//  * necessary data to a queue and handle it from a lower priority task. */
// void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
// {
//     espnow_event_t evt;
//     espnow_event_send_cb_t *send_cb = &evt.info.send_cb;

//     if (mac_addr == NULL) {
//         ESP_LOGE(TAG, "Send cb arg error");
//         return;
//     }

//     evt.id = EXAMPLE_ESPNOW_SEND_CB;
//     memcpy(send_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
//     send_cb->status = status;
//     if (xQueueSend(s_example_espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE) {
//         ESP_LOGW(TAG, "Send send queue fail");
//     }
// }


// void example_espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
// {
//     espnow_event_t evt;
//     espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;

//     if (mac_addr == NULL || data == NULL || len <= 0) {
//         ESP_LOGE(TAG, "Receive cb arg error");
//         return;
//     }

//     evt.id = EXAMPLE_ESPNOW_RECV_CB;
//     memcpy(recv_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
//     recv_cb->data = malloc(len);
//     if (recv_cb->data == NULL) {
//         ESP_LOGE(TAG, "Malloc receive data fail");
//         return;
//     }
//     memcpy(recv_cb->data, data, len);
//     recv_cb->data_len = len;
//     if (xQueueSend(s_example_espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE) {
//         ESP_LOGW(TAG, "Send receive queue fail");
//         free(recv_cb->data);
//     }
// }

// void example_espnow_data_prepare(send_param_t *send_param)
// {
//     espnow_data_t *buf = (espnow_data_t *)send_param->buffer;

//     assert(send_param->buf_len >= sizeof(espnow_data_t));

//     buf->dev_type = send_param->dev_type;
//     buf->crc = 0;
//     memcpy(buf->payload, send_param->data_to_send, send_param->data_len);
//     buf->crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, send_param->buf_len);
// }

// /* Parse received ESPNOW data. */
// int espnow_data_parse(uint8_t *data, uint16_t data_len, uint8_t *recv_dev_type)
// {
//     espnow_data_t *buf = (espnow_data_t *)data;
//     uint16_t crc, crc_cal = 0;

//     if (data_len < sizeof(espnow_data_t)) {
//         ESP_LOGE(TAG, "Receive ESPNOW data too short, len:%d", data_len);
//         return -1;
//     }
//     *recv_dev_type = buf->dev_type;
//     crc = buf->crc; 
//     buf->crc = 0;
//     crc_cal = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, data_len);

//     if (crc_cal != crc) {
//         ESP_LOGE(TAG, "CRC does not matchd");
//         return -1;
//     }



// //     //ESP_LOGI(TAG, "tak jak :%u", ((lux_data_t*)(buf->payload))->bat_percentage);

// //     switch(buf->dev_type){
// //         case LUXMETER:
// // ESP_LOGI(TAG, "oficial len len:%d", buf->payload_len);
// // ESP_LOGI(TAG, "real len len:%d", sizeof(lux_data_t));

// //             assert(buf->payload_len == sizeof(lux_data_t));
            
// //             return 1;

// //         default:
// //             ESP_LOGE(TAG, "Receive unknown device");

// //     }

//     return 1;
// }


// /* Prepare ESPNOW data to be sent. */
// void espnow_data_prepare(send_param_t *send_param)
// {
//     espnow_data_t *buf = (espnow_data_t *)send_param->buffer;

//     assert(send_param->buf_len >= sizeof(espnow_data_t));

//     buf->dev_type = send_param->dev_type;
//     buf->payload_len = send_param->data_len;
//     buf->crc = 0;
//     memcpy(buf->payload, send_param->data_to_send, send_param->data_len);
//     buf->crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, send_param->buf_len);

//     //todo na zkalade typu by mohlo switchovat jak velka je payload len
// }


// void get_message(uint8_t *data, void ** message){
//     espnow_data_t *buf = (espnow_data_t *)data;
//     memcpy(*message, buf->payload, buf->payload_len);
//     //todo  free?
// }



// static void my_espnow_deinit(send_param_t *send_param)
// {
//     free(send_param->buffer);
//     free(send_param);
//     //todo vSemaphoreDelete(s_espnow_queue);
//     esp_now_deinit();
// }



// void espnow_bridge_task(void *pvParameter)
// {
//     espnow_event_t evt;
//     uint8_t recv_dev_type = SWITCH;
//     bool is_broadcast = false;
//     int ret;

// //pro poslani dat
//     //example_espnow_data_prepare(send_param);

//     //ESP_LOGI(TAG, "Start sending broadcast data");




   
//     /* Start sending broadcast ESPNOW data. */
//     ESP_LOGI(TAG, "1");
//     send_param_t *send_param = (send_param_t *)pvParameter;
//     if (esp_now_send(send_param->dest_mac, send_param->buffer, send_param->buf_len) != ESP_OK) {
//         ESP_LOGI(TAG, "2");
//         ESP_LOGE(TAG, "Send error");
//         my_espnow_deinit(send_param);
//         vTaskDelete(NULL);
//     }
//     ESP_LOGI(TAG, "3");


//     vTaskDelay(pdMS_TO_TICKS(100));
//     while (xQueueReceive(s_espnow_queue, &evt, portMAX_DELAY) == pdTRUE) {
//         switch (evt.id) {
//             case ESPNOW_SEND_CB:
//             {
//                 ESP_LOGI(TAG, "4");
//                 espnow_event_send_cb_t *send_cb = &evt.info.send_cb;
//                 is_broadcast = IS_BROADCAST_ADDR(send_cb->mac_addr);

//                 ESP_LOGI(TAG, "Send data to "MACSTR", status1: %d", MAC2STR(send_cb->mac_addr), send_cb->status);

//                 if (!is_broadcast) {
//                     ESP_LOGI(TAG, "recieved non broadcast address");
//                 }
//                 break;
//             }

        
//             case ESPNOW_RECV_CB:{
//                 espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
//                 ESP_LOGI(TAG, "Receive broadcast data from: "MACSTR", len: %d", MAC2STR(recv_cb->mac_addr), recv_cb->data_len);
//                 ret = espnow_data_parse(recv_cb->data, recv_cb->data_len, &recv_dev_type);
//                 if(ret != 1){
//                     ESP_LOGE(TAG, "Data cannot be parsed!");
//                     free(recv_cb->data);
//                     break;
//                 }     

//                 switch(recv_dev_type){
//                     case LUXMETER:{
//                         lux_data_t lux_data = {
//                             .lux = 0,
//                             .bat_percentage=0 
//                         };
//                         lux_data_t* lux_data_p = &lux_data;
//                         get_message(recv_cb->data, (void*)&lux_data_p);
//                         free(recv_cb->data);              
                       
//                         ESP_LOGI(TAG, "     Data from LUXMETER");
//                         ESP_LOGI(TAG, "     LUX: %f", lux_data.lux);
//                         ESP_LOGI(TAG, "     BAT: %u",lux_data.bat_percentage);
//                         ESP_LOGI(TAG, "");
//                         break;
//                     }
                        
                
//                     case THERMOMETER:{
//                         therm_data_t therm_data = {
//                             .temperature = 0,
//                             .ph = 0,
//                             .bat_percentage = 0 
//                         };
//                         therm_data_t* therm_data_p = &therm_data;
//                         get_message(recv_cb->data, (void*)&therm_data_p);
//                         free(recv_cb->data); 
                                  
//                         ESP_LOGI(TAG, "     Data from THERMOMETER");
//                         ESP_LOGI(TAG, "     TEMP: %f", therm_data.temperature);
//                         ESP_LOGI(TAG, "     PH: %f", therm_data.ph);
//                         ESP_LOGI(TAG, "     BAT: %u", therm_data.bat_percentage);
//                         ESP_LOGI(TAG, "");
//                         break;
//                     }
                        

//                     case SWITCH:{
//                         switch_data_t switch_data = {
                            
//                             .flow = 33,
//                             .pressure = 33,
//                             .switch_status = false,
//                         };
//                         switch_data_t* switch_data_p = &switch_data;
//                         get_message(recv_cb->data, (void*)&switch_data_p);
//                         free(recv_cb->data);

//                         ESP_LOGI(TAG, "     Data from SWITCH");
//                         ESP_LOGI(TAG, "     SWITCHED: %s", switch_data.switch_status ? "ON" : "OFF");
//                         ESP_LOGI(TAG, "     WATTER FLOW: %d dm^3", switch_data.flow);
//                         ESP_LOGI(TAG, "     WATTER PRESSURE: %d Pa", switch_data.pressure);
//                         ESP_LOGI(TAG, "");
//                         break;
//                     }
                        

//                     default:{
//                         ESP_LOGE(TAG, "Unknown type of device!");
//                         break;
//                     }
                        
//                 }
//                 break;
//             }
                   
                   
                   
                   
                   
                   
                   
//             //         /* If MAC address does not exist in peer list, add it to peer list. */
//             //         if (esp_now_is_peer_exist(recv_cb->mac_addr) == false) {
//             //             esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
//             //             if (peer == NULL) {
//             //                 ESP_LOGE(TAG, "Malloc peer information fail");
//             //                 example_espnow_deinit(send_param);
//             //                 vTaskDelete(NULL);
//             //             }
//             //             memset(peer, 0, sizeof(esp_now_peer_info_t));
//             //             peer->channel = CONFIG_ESPNOW_CHANNEL;
//             //             peer->ifidx = ESPNOW_WIFI_IF;
//             //             peer->encrypt = true;
//             //             memcpy(peer->lmk, CONFIG_ESPNOW_LMK, ESP_NOW_KEY_LEN);
//             //             memcpy(peer->peer_addr, recv_cb->mac_addr, ESP_NOW_ETH_ALEN);
//             //             ESP_ERROR_CHECK( esp_now_add_peer(peer) );
//             //             free(peer);
//             //         }

//             //         /* Indicates that the device has received broadcast ESPNOW data. */
//             //         if (send_param->state == 0) {
//             //             send_param->state = 1;
//             //         }

//             //         /* If receive broadcast ESPNOW data which indicates that the other device has received
//             //          * broadcast ESPNOW data and the local magic number is bigger than that in the received
//             //          * broadcast ESPNOW data, stop sending broadcast ESPNOW data and start sending unicast
//             //          * ESPNOW data.
//             //          */
//             //         if (recv_state == 1) {
//             //             /* The device which has the bigger magic number sends ESPNOW data, the other one
//             //              * receives ESPNOW data.
//             //              */
//             //             if (send_param->unicast == false && send_param->magic >= recv_magic) {
//             //         	    ESP_LOGI(TAG, "Start sending unicast data");
//             //         	    ESP_LOGI(TAG, "send data to "MACSTR"", MAC2STR(recv_cb->mac_addr));

//             //         	    /* Start sending unicast ESPNOW data. */
//             //                 memcpy(send_param->dest_mac, recv_cb->mac_addr, ESP_NOW_ETH_ALEN);
//             //                 espnow_data_prepare(send_param);
//             //                 if (esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len) != ESP_OK) {
//             //                     ESP_LOGE(TAG, "Send error");
//             //                     example_espnow_deinit(send_param);
//             //                     vTaskDelete(NULL);
//             //                 }
//             //                 else {
//             //                     send_param->broadcast = false;
//             //                     send_param->unicast = true;
//             //                 }
//             //             }
//             //         }
//             //     }
//             //     else if (ret == EXAMPLE_ESPNOW_DATA_UNICAST) {
//             //         ESP_LOGI(TAG, "Receive %dth unicast data from: "MACSTR", len: %d", recv_seq, MAC2STR(recv_cb->mac_addr), recv_cb->data_len);

//             //         /* If receive unicast ESPNOW data, also stop sending broadcast ESPNOW data. */
//             //         send_param->broadcast = false;
//             //     }
//             //     else {
//             //         ESP_LOGI(TAG, "Receive error data from: "MACSTR"", MAC2STR(recv_cb->mac_addr));
//             //     }
//             //     break;
//             // }
//             default:
//             {
//                 ESP_LOGE(TAG, "Callback type error: %d", evt.id);
//                 break;
//             }
//         }
//     }
// }



// // esp_err_t sendData(lux_data_t * data){
// //     send_param_t *send_param;

// //     s_example_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(espnow_event_t));
// //     if (s_example_espnow_queue == NULL) {
// //         ESP_LOGE(TAG, "Create mutex fail");
// //         return ESP_FAIL;
// //     }

// //     /* Initialize ESPNOW and register sending and receiving callback function. */
// //     ESP_ERROR_CHECK( esp_now_init() );
// //     ESP_ERROR_CHECK( esp_now_register_send_cb(espnow_send_cb) );

// //     /* Set primary master key. */
// //     ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );
// //     /* Add broadcast peer information to peer list. */
// //     esp_now_peer_info_t *broadcast_peer = malloc(sizeof(esp_now_peer_info_t));
// //     if (broadcast_peer == NULL) {
// //         ESP_LOGE(TAG, "Malloc peer information fail");
// //         vSemaphoreDelete(s_example_espnow_queue);
// //         esp_now_deinit();
// //         return ESP_FAIL;
// //     }
// //     memset(broadcast_peer, 0, sizeof(esp_now_peer_info_t));
// //     broadcast_peer->channel = CONFIG_ESPNOW_CHANNEL;
// //     broadcast_peer->ifidx = ESPNOW_WIFI_IF;//todo nema byt station?
// //     broadcast_peer->encrypt = false;//todo true?
// //     memcpy(broadcast_peer->peer_addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
// //     ESP_ERROR_CHECK( esp_now_add_peer(broadcast_peer) );
// //     free(broadcast_peer);

// // ESP_LOGI("DEBUG", "1");
// //     /* Initialize sending parameters. */
// //     send_param = malloc(sizeof(send_param_t));
// //     memset(send_param, 0, sizeof(send_param_t));
// //     if (send_param == NULL) {
// //         ESP_LOGE(TAG, "Malloc send parameter fail");
// //         vSemaphoreDelete(s_example_espnow_queue);
// //         esp_now_deinit();
// //         return ESP_FAIL;
// //     }
// //     send_param->dev_type = LUXMETER;
// //     send_param->delay = CONFIG_ESPNOW_SEND_DELAY;
// //         send_param->len = sizeof(lux_data_t);
// //     send_param->buffer = malloc(sizeof(lux_data_t));
// //     if (send_param->buffer == NULL) {
// //         ESP_LOGE(TAG, "Malloc send buffer fail");
// //         free(send_param);
// //         vSemaphoreDelete(s_example_espnow_queue);
// //         esp_now_deinit();
// //         return ESP_FAIL;
// //     }
// //     memcpy(send_param->dest_mac, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
// //     memcpy(send_param->buffer, data, sizeof(lux_data_t));
// //     espnow_data_prepare(send_param);
// // ESP_LOGI("DEBUG", "2");
// //     xTaskCreate(&send_task, "example_espnow_task", 2048, send_param, 4, NULL);

// //     return ESP_OK;

// // }