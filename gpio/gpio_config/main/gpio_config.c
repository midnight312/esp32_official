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
   gpio_config_t gpio_config = {
      .intr_type = GPIO_INTR_POSEDGE,
      .pull_down_en = 1,
      .pull_up_en = 0,
      .mode = GPIO_MODE_INPUT,
      .pin_bit_mask = (1ULL << SWITCH_PIN) | (1ULL << 2);
   }; 
   gpio_config_t(&gpio_config);

    interputQueue = xQueueCreate(10, sizeof(int));
    xTaskCreate(buttonBushedTask, "press button", 512, NULL, 1, NULL);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(SWITCH_PIN, gpio_isr_handler, (void *)SWITCH_PIN);
   
}
