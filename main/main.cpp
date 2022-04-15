/************************************************************
 *                                                          *
 *      Firmware for bridge unit in SMPool system           *
 *                                                          *
 *      File:       main.c                                  *
 *      Author:     Marek Stastny                           *
 *      Created:    2022                                    *
 *                                                          *
 ************************************************************/


#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager



// #include <stdio.h>
// #include "freertos.h"
// #include "freertos/task.h"
 #include "nvs.h"
// #include "mqtt_client.h"



 #include "esp_log.h"
// #include "mqtt_client.h"
// #include "esp_tls.h"
// #include "esp_ota_ops.h"
// #include <sys/param.h>
// #include "shared.h"
#include "Arduino.h"

#include "WiFi.h"
#include "esp_private/wifi.h"



// #include "WiFiSettings.h"


// #include <SPIFFS.h>
// #include <WiFiSettings.h>
// #include "Arduino.h"



extern "C" {
    #include "bridge_core.h"
  void app_main();
}

//#include "bridge_core.h"

#include "my_espnow.h"

#include "esp_spiffs.h"




const char* TAG = "MAIN";


// Status LED
const uint32_t LED_PIN = 2;
#define LED_ON  LOW
#define LED_OFF HIGH




#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"










//void esp_now_setup() {
//  WiFi.mode(WIFI_AP_STA);

//   Serial.print("Got IP: ");  Serial.println(WiFi.localIP());

//   // This is the mac address
//   Serial.print("AP MAC: "); Serial.println(WiFi.softAPmacAddress());
//   Serial.print("Wifi channel: "); Serial.println(WiFi.channel());

//   //Force espnow to use a specific channel
//   ESP_ERROR_CHECK(esp_wifi_set_promiscuous(true));

//   uint8_t primaryChan = WiFi.channel();
//   wifi_second_chan_t secondChan = WIFI_SECOND_CHAN_NONE;
//   esp_wifi_set_channel(primaryChan, secondChan);

//   printf("Setting High Spees Mode...[");
//   int ret = esp_wifi_internal_set_fix_rate(WIFI_IF_AP, 1, WIFI_PHY_RATE_MCS7_SGI);
//   printf("%d]\r\n", ret);
  
//   ESP_ERROR_CHECK(esp_wifi_set_promiscuous(false));

  //WiFi.disconnect();

  //Serial.print("Wifi Channel: "); Serial.println(WiFi.channel());

//   if (esp_now_init() == ESP_OK) {
//     Serial.println("ESPNow Init Success!");
//   } else {
//     Serial.println("ESPNow Init Failed....");
//   }

//   //Add the slave node to this master node
//   create_broadcast_mac();
//   create_client_macs(DEVICE_ID_SHARED, DEVICE_COUNT_SHARED);
  
//   esp_now_register_recv_cb(OnDataRecv);
//   esp_now_register_send_cb(OnDataSent);

//   Serial.print("AMPDU: ");
//   Serial.println(CONFIG_ESP32_WIFI_AMPDU_RX_ENABLED); // CONFIG_ESP32_WIFI_AMPDU_TX_ENABLED
//   CONFIG_ESPNOW_DONE = true;
//}
//}













void app_main(void)
{
     initArduino();
    // // ESP_ERROR_CHECK(esp_event_loop_create_default());

    // // // Initialize NVS
    // // esp_err_t ret = nvs_flash_init();
    // // if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // //     ESP_ERROR_CHECK( nvs_flash_erase() );
    // //     ret = nvs_flash_init();
    // // }

    // // esp_vfs_spiffs_conf_t conf = {
    // //   .base_path = "/spiffs",
    // //   .partition_label = NULL,
    // //   .max_files = 5,
    // //   .format_if_mount_failed = true
    // // };

    // // // Use settings defined above to initialize and mount SPIFFS filesystem.
    // // // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    // // esp_err_t ret = esp_vfs_spiffs_register(&conf);

    // // if (ret != ESP_OK) {
    // //     if (ret == ESP_FAIL) {
    // //         ESP_LOGE(TAG, "Failed to mount or format filesystem");
    // //     } else if (ret == ESP_ERR_NOT_FOUND) {
    // //         ESP_LOGE(TAG, "Failed to find SPIFFS partition");
    // //     } else {
    // //         ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
    // //     }
    // //     return;
    // // }


    // Serial.begin(115200);
    // SPIFFS.begin(true);  // Will format on the first run after failing to mount

    // pinMode(LED_PIN, OUTPUT);

    // // Set custom callback functions
    // WiFiSettings.onSuccess  = []() {
    //     digitalWrite(LED_PIN, LED_ON); // Turn LED on
    // };
    // WiFiSettings.onFailure  = []() {
    //     digitalWrite(LED_PIN, LED_OFF); // Turn LED off
    // };
    // WiFiSettings.onWaitLoop = []() {
    //     digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Toggle LED
    //     return 500; // Delay next function call by 500ms
    // };

    // // Callback functions do not have to be lambda's, e.g.
    // // WiFiSettings.onPortalWaitLoop = blink;

    // // Define custom settings saved by WifiSettings
    // // These will return the default if nothing was set before
    // String host = WiFiSettings.string( "server_host", "default.example.org");
    // int    port = WiFiSettings.integer("server_port", 443);

    // // Connect to WiFi with a timeout of 30 seconds
    // // Launches the portal if the connection failed
    // WiFiSettings.connect(true, 30);

    // //radio setup
    // esp_now_setup();

    // // Initialize NVS
    // // esp_err_t ret = nvs_flash_init();
    // // if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // //     ESP_ERROR_CHECK( nvs_flash_erase() );
    // //     ret = nvs_flash_init();
    // // }

    // //wifi_init();





//WIFI MANAGER BEGIN
WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    // it is a good practice to make sure your code sets wifi mode how you want it.

    // put your setup code here, to run once:
    Serial.begin(115200);
    
    WiFiManager wm;
    bool res;
    res = wm.autoConnect("SMpool","SMpoolpass1"); // password protected ap
    if(!res) {
        Serial.println("Failed to connect");
        esp_restart();
    } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("Connected to WiFi AP");
        //radio setup
        WiFi.mode(WIFI_AP_STA);
        bridge_core();
    }
//WIFI MANAGER END

    while(1){
        vTaskDelay(pdMS_TO_TICKS(1000000));
    }












    // SPIFFS.begin(true);  // Will format on the first run after failing to mount

    // pinMode(LED_PIN, OUTPUT);

    // // Set custom callback functions
    // WiFiSettings.onSuccess  = []() {
    //     digitalWrite(LED_PIN, LED_ON); // Turn LED on
    // };
    // WiFiSettings.onFailure  = []() {
    //     digitalWrite(LED_PIN, LED_OFF); // Turn LED off
    // };
    // WiFiSettings.onWaitLoop = []() {
    //     //digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // Toggle LED
    //     return 500; // Delay next function call by 500ms
    // };
    // WiFiSettings.onPortal = []() {
    //     setup_ota();
    // };
    




    // ESP_ERROR_CHECK(nvs_flash_init());
    // ESP_ERROR_CHECK(esp_netif_init());
    // ESP_ERROR_CHECK(esp_event_loop_create_default());




  
    // wifi_manager_start();
    // wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);
    // vTaskDelay(pdMS_TO_TICKS(5000));
    
    // const esp_mqtt_client_config_t mqtt_cfg = {
    //     .uri = CONFIG_BROKER_URI//,
    //     //.username = "Smpool",
    //     //.password = "Smpoolpass1"
    //     // .user_context = (void *)your_context
    // };
    // esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    // esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);


    // // Initialize NVS
    // esp_err_t ret = nvs_flash_init();
    // if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    //     ESP_ERROR_CHECK( nvs_flash_erase() );
    //     ret = nvs_flash_init();
    // }
    // wifi_init();

    //esp_wifi_set_ps(WIFI_PS_NONE);







    




    // ESP_ERROR_CHECK(example_connect());
    // const esp_mqtt_client_config_t mqtt_cfg = {
    // .uri = CONFIG_BROKER_URI//,
    // //.username = "Smpool",
    // //.password = "Smpoolpass1"
    // // .user_context = (void *)your_context
    // };
    // esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    // esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    // esp_mqtt_client_start(client);




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