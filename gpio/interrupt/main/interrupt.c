/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
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
xQueueHandle interputQueue;

static void IRAM_ATTR gpio_isr_handler(void *args)
{
   int pinNumber = (int)args;
   xQueueSendFromISR(interputQueue, pinNumber, NULL);
}

void buttonBushedTask(void *params)
{
   int pinNum;
   while(true)
   {
      if(xQueueReceive(interputQueue, &pinNum, portMAX_DELAY))
      {
         //dissable interrupt
         gpio_isr_handler_remove(pinNum);
         //wait some time while we check for button to be released
         do
         {
            vTaskDelay( 20 / portMAX_DELAY);
         }while(gpio_get_level(pinNum) == 1);
         //do some work
         printf("GPIO %d is pressed %d time. The state is\n",pinNum,gpio_get_level(SWITCH_PIN));
         //enable interrupt 
         gpio_isr_handler_add(pinNum, gpio_isr_handler, (void *)pinNum);
      }


   }
}



void app_main(void)
{
    gpio_pad_select_gpio(SWITCH_PIN);
    gpio_set_direction(SWITCH_PIN, GPIO_MODE_INPUT);
    gpio_pulldown_en(SWITCH_PIN);
    gpio_pullup_dis(SWITCH_PIN);
    gpio_set_intr_type(SWITCH_PIN, GPIO_INTR_POSEDGE);

    interputQueue = xQueueCreate(10, sizeof(int));
    xTaskCreate(buttonBushedTask, "press button", 512, NULL, 1, NULL);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(SWITCH_PIN, gpio_isr_handler, (void *)SWITCH_PIN);

}
