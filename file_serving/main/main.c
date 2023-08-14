/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* HTTP File Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_vfs_fat.h"
#include "protocol_examples_common.h"
#include "driver/gpio.h"
#include "file_serving_example_common.h"
#include <string.h>

#define SD_CARD
//#define SPIFFS

#ifdef CONFIG_EXAMPLE_MOUNT_SD_CARD
// SD_CARD
// GPIO 0x3FF4_4000 0x3FF4_4FFF 4 KB
#define BLINK_GPIO                  27    
#define CONFIG_BLINK_PERIOD_SHORT   250
#define CONFIG_BLINK_PERIOD_LONG    3000
#define PIN_NUM_MISO                2       // Built-in LED 0x3FF48430
#define PIN_NUM_MOSI                15
#define PIN_NUM_CLK                 14
#define PIN_NUM_CS                  13
#endif

#ifdef SPIFFS
#define BLINK_GPIO                  2       // Built-in LED 0x3FF48430
#define CONFIG_BLINK_PERIOD_SHORT   250
#define CONFIG_BLINK_PERIOD_LONG    3000
#define PIN_NUM_MISO                2 
#define PIN_NUM_MOSI                15
#define PIN_NUM_CLK                 14
#define PIN_NUM_CS                  13
#endif



/* This example demonstrates how to create file server
 * using esp_http_server. This file has only startup code.
 * Look in file_server.c for the implementation.
 */

static const char *TAG = "ESP32-SERVER";
static uint8_t s_led_state = 0;
uint64_t total_bytes, free_bytes;

static void configure_led (void) {
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

void blink_task (void *pvParameter)
{
    while (1) {
        // Short ON
        s_led_state = !s_led_state;
        gpio_set_level(BLINK_GPIO, s_led_state);
        vTaskDelay(CONFIG_BLINK_PERIOD_SHORT / portTICK_PERIOD_MS);
        // Short OFF
        s_led_state = !s_led_state;        
        gpio_set_level(BLINK_GPIO, s_led_state);
        vTaskDelay(CONFIG_BLINK_PERIOD_SHORT / portTICK_PERIOD_MS);
        // Short ON
        s_led_state = !s_led_state;
        gpio_set_level(BLINK_GPIO, s_led_state);
        vTaskDelay(CONFIG_BLINK_PERIOD_SHORT / portTICK_PERIOD_MS);
        // Long OFF
        s_led_state = !s_led_state;        
        gpio_set_level(BLINK_GPIO, s_led_state);
        vTaskDelay(CONFIG_BLINK_PERIOD_LONG / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
    configure_led();
    xTaskCreatePinnedToCore(&blink_task, "blink LED task", 2048, NULL, 5, NULL, 0);
    ESP_LOGI(TAG, "Starting example");
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Initialize file storage */
    const char* base_path = "/data";
    ESP_ERROR_CHECK(example_mount_storage(base_path));
    esp_vfs_fat_info(base_path, &total_bytes, &free_bytes);
    char card_total [16];
    char card_free [16];
    sprintf(card_total, "%lld", total_bytes/1024/1024);
    sprintf(card_free, "%lld", free_bytes/1024/1024);
    ESP_LOGI(TAG, "MAIN. SD Card Total: %sMB Free %sMB", card_total, card_free);
    

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    /* Start the file server */
    ESP_ERROR_CHECK(example_start_file_server(base_path));
    ESP_LOGI(TAG, "File server started");
}
