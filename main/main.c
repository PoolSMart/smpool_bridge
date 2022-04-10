/************************************************************
 *                                                          *
 *      Firmware for bridge unit in SMPool system           *
 *                                                          *
 *      File:       main.c                                  *
 *      Author:     Marek Stastny                           *
 *      Created:    2022                                    *
 *                                                          *
 ************************************************************/



#include <stdio.h>
#include <my_espnow.h>
#include "freertos/task.h"
#include "nvs.h"
#include "mqtt_client.h"
#include "esp_wifi.h"

static const char *TAG = "ESPNOW";





static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
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



void app_main(void)
{
    
    
    
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());   

    



    ESP_ERROR_CHECK(example_connect());
    const esp_mqtt_client_config_t mqtt_cfg = {
    .uri = "f32a355886424b0b9bea85ddd69e2007.s1.eu.hivemq.cloud",
    .username = "Smpool",
    .password = "Smpoolpass1"
    // .user_context = (void *)your_context
    };
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);




//     switch_data_t data;

//  // Initialize NVS
//   esp_err_t ret = nvs_flash_init();
//   if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
//       ESP_ERROR_CHECK( nvs_flash_erase() );
//       ret = nvs_flash_init();
//   }
//   ESP_ERROR_CHECK( ret );

//   example_wifi_init();
//   //example_espnow_init();
//   vTaskDelay(2000/portTICK_RATE_MS);




//     lux_data_t measured_data;
//     while(true){
//         //sendData(&measured_data);
//         //vTaskDelay(2000/portTICK_RATE_MS);
    



//         //Recieving esp now messages
//         send_param_t *send_param;

//         s_example_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(espnow_event_t));
//         if (s_example_espnow_queue == NULL) {
//             ESP_LOGE(TAG, "Create mutex fail");
//             return ;
//         }
//         //vTaskDelay(5000 / portTICK_RATE_MS);

//         /* Initialize ESPNOW and register sending and receiving callback function. */
//         ESP_ERROR_CHECK( esp_now_init() );
//         ESP_ERROR_CHECK( esp_now_register_send_cb(espnow_send_cb) );
//         ESP_ERROR_CHECK( esp_now_register_recv_cb(example_espnow_recv_cb) );

//         /* Set primary master key. */
//         ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );
//         /* Add broadcast peer information to peer list. */
//         esp_now_peer_info_t *broadcast_peer = malloc(sizeof(esp_now_peer_info_t));
//         if (broadcast_peer == NULL) {
//             ESP_LOGE(TAG, "Malloc peer information fail");
//             vSemaphoreDelete(s_example_espnow_queue);
//             esp_now_deinit();
//             return ;
//         }
//         memset(broadcast_peer, 0, sizeof(esp_now_peer_info_t));
//         broadcast_peer->channel = CONFIG_ESPNOW_CHANNEL;
//         broadcast_peer->ifidx = ESPNOW_WIFI_IF;//todo nema byt station?
//         broadcast_peer->encrypt = false;//todo true?
//         memcpy(broadcast_peer->peer_addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
//         ESP_ERROR_CHECK( esp_now_add_peer(broadcast_peer) );
//         free(broadcast_peer);

//         /* Initialize sending parameters. */
//         send_param = malloc(sizeof(send_param_t));
//         memset(send_param, 0, sizeof(send_param_t));
//         if (send_param == NULL) {
//             ESP_LOGE(TAG, "Malloc send parameter fail");
//             vSemaphoreDelete(s_example_espnow_queue);
//             esp_now_deinit();
//             return;
//         }
//         send_param->buf_len = sizeof(espnow_data_t) + sizeof(switch_data_t) - 1;
//         send_param->buffer = malloc(sizeof(espnow_data_t) + sizeof(switch_data_t) - 1);
//     if (send_param->buffer == NULL) {
//         ESP_LOGE(TAG, "Malloc send buffer fail");
//         free(send_param);
//         vSemaphoreDelete(s_example_espnow_queue);
//         esp_now_deinit();
//         return ;
//     }

// //todo remove
//     u_int8_t param = 1;
//     send_param->data_to_send = &param;
//     send_param->dev_type = SWITCH;
//     send_param->data_len = sizeof(u_int8_t );
//     memcpy(send_param->dest_mac, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
//     espnow_data_prepare(send_param);


//     // send_param->data_to_send = &data;
//     // send_param->data_len = sizeof(switch_data_t );
//     // memcpy(send_param->dest_mac, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
    
//     xTaskCreate(&espnow_bridge_task, "bridge task", 2048, send_param, 4, NULL);
//     while(1){
//         vTaskDelay(100000/portTICK_RATE_MS);
//     }
//     }


}



// //todo do send task pred odeslanim zpravy nasetupovat toto:
//     send_param->buffer = malloc(sizeof(lux_data_t));
//     if (send_param->buffer == NULL) {
//         ESP_LOGE(TAG, "Malloc send buffer fail");`
//         free(send_param);
//         vSemaphoreDelete(s_example_espnow_queue);
//         esp_now_deinit();
//         return ESP_FAIL;
//     }
//     memcpy(send_param->buffer, data, sizeof(lux_data_t));
//     espnow_data_prepare(send_param);



//     //resend z tasku vetev send callback 
//     /* Delay a while before sending the next data. */
//                 if (send_param->delay > 0) {
//                     vTaskDelay(send_param->delay/portTICK_RATE_MS);
//                 }

//                 ESP_LOGI(TAG, "send data to "MACSTR"", MAC2STR(send_cb->mac_addr));

//                 memcpy(send_param->dest_mac, send_cb->mac_addr, ESP_NOW_ETH_ALEN);
//                 espnow_data_prepare(send_param);

//                 /* Send the next data after the previous data is sent. */
//                 if (esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len) != ESP_OK) {
//                     ESP_LOGE(TAG, "Send error");
//                     example_espnow_deinit(send_param);
//                     vTaskDelete(NULL);
//                 }