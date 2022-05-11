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
#include "nvs.h"
#include "esp_log.h"
#include "Arduino.h"
#include "esp_private/wifi.h"
#include "my_espnow.h"
#include "esp_spiffs.h"
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"


extern "C" {
    #include "bridge_core.h"
  void app_main();
}

const char* TAG = "MAIN";


// Status LED

#define LED_ON  LOW
#define LED_OFF HIGH
#define INFO_LED_PIN    GPIO_NUM_5


//intitialize output pin
//pin is set to low
void initialize_output_pin(gpio_num_t pin){
  gpio_reset_pin(pin);
  gpio_set_direction(pin, GPIO_MODE_OUTPUT);
  gpio_set_level(pin, LED_OFF);
}


void app_main(void){
initialize_output_pin(INFO_LED_PIN);
gpio_set_level(INFO_LED_PIN, LED_ON);
initArduino();
//Autoconect to WiFi
WiFi.mode(WIFI_STA);
    Serial.begin(115200);
    WiFiManager wm;
    if(!wm.autoConnect("SMpool","SMpoolpass1")) {
        ESP_LOGI("WIFIManager","Failed to connect");
        esp_restart();
    } 

    ESP_LOGI("WIFIManager","Connected to WiFi AP");
    WiFi.mode(WIFI_AP_STA);
    gpio_set_level(INFO_LED_PIN, LED_OFF);
    bridge_core(); //Start bridge main job

    // another tasks are running, do nothing
    while(1){
        vTaskDelay(pdMS_TO_TICKS(1000000));
    }
}