/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

static const char *TAG = "example";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define LED_PIN 2
#define TIM_DELAY 1000

static uint8_t s_led_state = 1;



void app_main(void)
{
    gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    while (1) 
    {
       ESP_LOGI(TAG, "LED %s\n",(s_led_state == true)?"ON":"OFF");
       gpio_set_level(LED_PIN, s_led_state);
       s_led_state = 1 - s_led_state;
       vTaskDelay(TIM_DELAY / portTICK_PERIOD_MS);
    }
}
