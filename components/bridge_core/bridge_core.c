


#include "my_espnow.h"
#include "my_mqtt.h"





#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "mqtt_client.h"
#include "esp_wifi.h"




#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include <sys/param.h>




typedef enum{
  OFF,
  ON
} switch_state_t;




static const char *TAG = "ESPNOW";
uint8_t broadcast_mac_s[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
send_param_t* send_param_p;

extern xQueueHandle s_espnow_queue;

#if CONFIG_BROKER_CERTIFICATE_OVERRIDDEN == 1
static const uint8_t mqtt_eclipseprojects_io_pem_start[]  = "-----BEGIN CERTIFICATE-----\n" CONFIG_BROKER_CERTIFICATE_OVERRIDE "\n-----END CERTIFICATE-----";
#else
extern const uint8_t mqtt_eclipseprojects_io_pem_start[]   asm("_binary_mqtt_eclipseprojects_io_pem_start");
#endif
extern const uint8_t mqtt_eclipseprojects_io_pem_end[]   asm("_binary_mqtt_eclipseprojects_io_pem_end");
esp_mqtt_client_handle_t mqtt_client;



static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, "SMpool/pool/switch/comands", 2);
        ESP_LOGI(TAG, "Sent subscribe successful, msg_id=%d.", msg_id);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        vTaskDelay(pdMS_TO_TICKS(60000));
        esp_restart();
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        if (strncmp(event->data, "send binary please", event->data_len) == 0) {
            ESP_LOGI(TAG, "Sending the binary");
            //send_binary(client);
        }
        if(strncmp(event->topic, "SMpool/pool/switch/comands", event->topic_len) == 0){
            switch_request_t command;
            if(strncmp(event->data, "ON_", event->data_len) == 0){
                command = ON;
            }
            else{
                command = OFF;
            }
            send_param_p->dev_type = SWITCH;      
            send_param_p->data_to_send = (uint8_t*) &command;
            send_param_p->data_len = sizeof(switch_request_t);
            my_send(send_param_p);
        }
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            ESP_LOGI(TAG, "Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                     strerror(event->error_handle->esp_transport_sock_errno));
        } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
            ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
        } else {
            ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}


void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URI,
        .cert_pem = (const char *)mqtt_eclipseprojects_io_pem_start,
    };
    
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}


void myBridgeEspNowTask(void* send_param_pv){
    espnow_event_t evt;
    uint8_t recv_dev_type = SWITCH;
    int ret;
    send_param_t *send_param = (send_param_t*)send_param_pv;
    while (xQueueReceive(s_espnow_queue, &evt, portMAX_DELAY) == pdTRUE) {
        switch (evt.id) {
            case ESPNOW_SEND_CB:{
                espnow_event_send_cb_t *send_cb = &evt.info.send_cb;
                if(send_cb->status == ESP_NOW_SEND_SUCCESS){
                    ESP_LOGI(TAG, "Send succeeded.");
                }else{
                    ESP_LOGI(TAG, "Send failed.");
                }
                break;
            }
                

                // ESP_LOGI("ESP NOW INFO", "Send data: \n     pressure: %u\n     flow: %u\n     switched: %s",
                //     ((switch_data_t*)(((espnow_data_t*)send_param->buffer)->payload))->pressure,
                //     ((switch_data_t*)(((espnow_data_t*)send_param->buffer)->payload))->flow,
                //     ((switch_data_t*)(((espnow_data_t*)send_param->buffer)->payload))->switch_status ? "ON" : "OFF" 
                // );
                // espnow_event_send_cb_t *send_cb = &evt.info.send_cb;
                // break;

            case ESPNOW_RECV_CB:{
                espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
                ESP_LOGI(TAG, "Receive broadcast data from: "MACSTR", len: %d", MAC2STR(recv_cb->mac_addr), recv_cb->data_len);
                ret = espnow_data_parse(recv_cb->data, recv_cb->data_len, &recv_dev_type);
                if(ret != 1){
                    ESP_LOGE(TAG, "Data cannot be parsed!");
                    free(recv_cb->data);
                    break;
                }     

                switch(recv_dev_type){
                    case LUXMETER:{
                        lux_data_t lux_data = {
                            .lux = 0,
                            .bat_percentage=0 
                        };
                        lux_data_t* lux_data_p = &lux_data;
                        get_message(recv_cb->data, (void*)&lux_data_p);
                        free(recv_cb->data);              
                       
                        ESP_LOGI(TAG, "     Data from LUXMETER");
                        ESP_LOGI(TAG, "     LUX: %f", lux_data.lux);
                        ESP_LOGI(TAG, "     BAT: %u",lux_data.bat_percentage);
                        ESP_LOGI(TAG, "");

                        char* msg = (char*)malloc( sizeof("000"));
                        sprintf(msg, "%03d",lux_data.bat_percentage);
                        int msg_id =  esp_mqtt_client_publish(mqtt_client, "SMpool/pool/luxmeter/bat", msg, 0, 2, 0);
                        free(msg);

                        u_int32_t lux = lux_data.lux;
                        msg = (char*)malloc( sizeof("000000"));
                        sprintf(msg, "%06d",lux);
                        msg_id =  esp_mqtt_client_publish(mqtt_client, "SMpool/pool/luxmeter/lux", msg, 0, 2, 0);
                        free(msg);

                        break;
                    }
                        
                
                    case THERMOMETER:{
                        therm_data_t therm_data = {
                            .temperature = 0,
                            .ph = 0,
                            .bat_percentage = 0 
                        };
                        therm_data_t* therm_data_p = &therm_data;
                        get_message(recv_cb->data, (void*)&therm_data_p);
                        free(recv_cb->data); 
                                  
                        ESP_LOGI(TAG, "     Data from THERMOMETER");
                        ESP_LOGI(TAG, "     TEMP: %f", therm_data.temperature);
                        ESP_LOGI(TAG, "     PH: %f", therm_data.ph);
                        ESP_LOGI(TAG, "     BAT: %u", therm_data.bat_percentage);
                        ESP_LOGI(TAG, "");

                        char* msg = (char*)malloc( sizeof("000"));
                        sprintf(msg, "%03d",therm_data.bat_percentage);
                        int msg_id =  esp_mqtt_client_publish(mqtt_client, "SMpool/pool/thermometer/bat", msg, 0, 2, 0);
                        free(msg);

                        msg = (char*)malloc( sizeof("+00.0"));
                        sprintf(msg, "%+05.1f",therm_data.ph);
                        msg_id =  esp_mqtt_client_publish(mqtt_client, "SMpool/pool/thermometer/ph", msg, 0, 2, 0);

                        sprintf(msg, "%+05.1f",therm_data.temperature);
                        msg_id =  esp_mqtt_client_publish(mqtt_client, "SMpool/pool/thermometer/temperature", msg, 0, 2, 0);
                        free(msg);

                        break;
                    }
                        

                    case SWITCH:{
                        switch_data_t switch_data = {
                            .flow = 33,
                            .pressure = 33,
                            .switch_status = false,
                        };
                        switch_data_t* switch_data_p = &switch_data;
                        get_message(recv_cb->data, (void*)&switch_data_p);
                        free(recv_cb->data);

                        ESP_LOGI(TAG, "     Data from SWITCH");
                        ESP_LOGI(TAG, "     SWITCHED: %s", switch_data.switch_status ? "ON_" : "OFF");
                        ESP_LOGI(TAG, "     WATTER FLOW: %d dm^3", switch_data.flow);
                        ESP_LOGI(TAG, "     WATTER PRESSURE: %d Pa", switch_data.pressure);
                        ESP_LOGI(TAG, "");

                        char* msg  =  (char*)malloc(sizeof("ON_"));
                        sprintf(msg, "%s",switch_data.switch_status ? "ON_" : "OFF");
                        int msg_id =  esp_mqtt_client_publish(mqtt_client, "SMpool/pool/switch/status", msg, 0, 2, 0);
                        free(msg);

                        msg = (char*)malloc( sizeof("000000"));
                        sprintf(msg, "%06d",switch_data.pressure);
                        msg_id =  esp_mqtt_client_publish(mqtt_client, "SMpool/pool/switch/pressure", msg, 0, 2, 0);
                        free(msg);

                        msg = (char*)malloc( sizeof("00000"));
                        sprintf(msg, "%05d",switch_data.flow);
                        msg_id =  esp_mqtt_client_publish(mqtt_client, "SMpool/pool/switch/flow", msg, 0, 2, 0);
                        free(msg);

                        break;
                    }

                    case TOUCH:{
                        send_param_t *touch_send_param;
                        touch_send_param = malloc(sizeof(send_param_t));
                        if (touch_send_param == NULL) {
                            ESP_LOGE(TAG, "Malloc touch send_param failed");
                            break;
                        }
                        memset(touch_send_param, 0, sizeof(send_param_t));
                        touch_send_param->dev_type = TOUCH_RESPONSE;
                        touch_send_param->delay = CONFIG_ESPNOW_SEND_DELAY;
                        touch_send_param->buf_len = sizeof(espnow_data_t)+1;
                        touch_send_param->buffer = malloc(sizeof(espnow_data_t)+1);
                        if (touch_send_param->buffer == NULL) {
                            ESP_LOGE(TAG, "Malloc send buffer fail");
                            free(touch_send_param);
                            break;
                        }
                        memcpy(touch_send_param->dest_mac, broadcast_mac_s, ESP_NOW_ETH_ALEN);
                            my_send(touch_send_param);
                        ESP_LOGI(TAG, "TOUCH");
                        break;
                    }
                        
                    default:
                        ESP_LOGE(TAG, "Unknown type of device!");
                        break;
                    
                }
                break;
            }

            default:
                ESP_LOGE(TAG, "Callback type error: %d", evt.id);
                break;       
        }
    }
    vTaskDelete(NULL);
}

void bridge_core(){
    mqtt_app_start();
    
    myEspNowSetup(&send_param_p, sizeof(switch_data_t), BRIDGE);
    xTaskCreate(&myBridgeEspNowTask, "MyEspNowTask", 4096, send_param_p, 9, NULL);
    while(true){
        ESP_LOGI("","end loop");
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}