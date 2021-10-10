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
#define SWITCH_PIN 15
#define TIM_DELAY 1000

static uint8_t s_switch_state = 0;






void app_main(void)
{
    gpio_pad_select_gpio(SWITCH_PIN);
    gpio_set_direction(SWITCH_PIN, GPIO_MODE_INPUT);
    gpio_pulldown_en(SWITCH_PIN);
    while (1) 
    {
       s_switch_state = gpio_get_level(SWITCH_PIN);
       ESP_LOGI(TAG, "SWITCH %s\n",(s_switch_state == true)?"ON":"OFF");
       vTaskDelay(TIM_DELAY / portTICK_PERIOD_MS);
    }
}
