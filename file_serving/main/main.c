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
#include "protocol_examples_common.h"
#include "driver/gpio.h"
#include "file_serving_example_common.h"

#define BLINK_GPIO                  2    // Built-in LED
#define CONFIG_BLINK_PERIOD_SHORT   250
#define CONFIG_BLINK_PERIOD_LONG    3000

/* This example demonstrates how to create file server
 * using esp_http_server. This file has only startup code.
 * Look in file_server.c for the implementation.
 */

static const char *TAG = "example";
static uint8_t s_led_state = 0;

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

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    /* Start the file server */
    ESP_ERROR_CHECK(example_start_file_server(base_path));
    ESP_LOGI(TAG, "File server started");
}
