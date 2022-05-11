/*************************************************************************
 *                                                                       *
 *      Component for comunication over esp_now                          *
 *                                                                       *
 *      File:       my_espnow.c                                          *
 *      Author:     Marek Stastny                                        *
 *      Created:    2022                                                 *
 *                                                                       *
 * This code is based on exampple form ESP-IDF                           *
 * https://github.com/espressif/esp-idf/tree/master/examples/wifi/espnow *
 ************************************************************************/

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
