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


static const char *TAG = "ESPNOW";


void app_main(void)
{
    switch_data_t data;

 // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK( nvs_flash_erase() );
      ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK( ret );

  example_wifi_init();
  //example_espnow_init();
  vTaskDelay(2000/portTICK_RATE_MS);




    lux_data_t measured_data;
    while(true){
        //sendData(&measured_data);
        //vTaskDelay(2000/portTICK_RATE_MS);
    



        //Recieving esp now messages
        send_param_t *send_param;

        s_example_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(espnow_event_t));
        if (s_example_espnow_queue == NULL) {
            ESP_LOGE(TAG, "Create mutex fail");
            return ;
        }
        //vTaskDelay(5000 / portTICK_RATE_MS);

        /* Initialize ESPNOW and register sending and receiving callback function. */
        ESP_ERROR_CHECK( esp_now_init() );
        ESP_ERROR_CHECK( esp_now_register_send_cb(espnow_send_cb) );
        ESP_ERROR_CHECK( esp_now_register_recv_cb(example_espnow_recv_cb) );

        /* Set primary master key. */
        ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );
        /* Add broadcast peer information to peer list. */
        esp_now_peer_info_t *broadcast_peer = malloc(sizeof(esp_now_peer_info_t));
        if (broadcast_peer == NULL) {
            ESP_LOGE(TAG, "Malloc peer information fail");
            vSemaphoreDelete(s_example_espnow_queue);
            esp_now_deinit();
            return ;
        }
        memset(broadcast_peer, 0, sizeof(esp_now_peer_info_t));
        broadcast_peer->channel = CONFIG_ESPNOW_CHANNEL;
        broadcast_peer->ifidx = ESPNOW_WIFI_IF;//todo nema byt station?
        broadcast_peer->encrypt = false;//todo true?
        memcpy(broadcast_peer->peer_addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
        ESP_ERROR_CHECK( esp_now_add_peer(broadcast_peer) );
        free(broadcast_peer);

        /* Initialize sending parameters. */
        send_param = malloc(sizeof(send_param_t));
        memset(send_param, 0, sizeof(send_param_t));
        if (send_param == NULL) {
            ESP_LOGE(TAG, "Malloc send parameter fail");
            vSemaphoreDelete(s_example_espnow_queue);
            esp_now_deinit();
            return;
        }
        send_param->buf_len = sizeof(espnow_data_t) + sizeof(switch_data_t) - 1;
        send_param->buffer = malloc(sizeof(espnow_data_t) + sizeof(switch_data_t) - 1);
    if (send_param->buffer == NULL) {
        ESP_LOGE(TAG, "Malloc send buffer fail");
        free(send_param);
        vSemaphoreDelete(s_example_espnow_queue);
        esp_now_deinit();
        return ;
    }

//todo remove
    u_int8_t param = 1;
    send_param->data_to_send = &param;
    send_param->dev_type = SWITCH;
    send_param->data_len = sizeof(u_int8_t );
    memcpy(send_param->dest_mac, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
    espnow_data_prepare(send_param);


    // send_param->data_to_send = &data;
    // send_param->data_len = sizeof(switch_data_t );
    // memcpy(send_param->dest_mac, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
    
    xTaskCreate(&espnow_bridge_task, "bridge task", 2048, send_param, 4, NULL);
    while(1){
        vTaskDelay(100000/portTICK_RATE_MS);
    }
    }


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