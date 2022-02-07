#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "finger.h"
#include "cJSON.h"
#include "connect.h"
#include "ssd1306.h"
#include "fonts.h"
#include "gpio.h"
#include "mqtt_client.h"
#include "user_uart.h"
#include "driver/uart.h"
#include <time.h>

#define TAG "NOTIFI"






void app_main(void)
{
  init_UART();


  
}
