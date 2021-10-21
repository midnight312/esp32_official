#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"

#define SSID            "Tenda_DFB070"
#define PASSWORD        "12345678"

xSemaphoreHandle onConnectionHandler;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id)
    {
        case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        printf("connecting ...\n");
        break;

        case SYSTEM_EVENT_STA_CONNECTED:
        printf("connected ...\n");
        break;

        case SYSTEM_EVENT_STA_GOT_IP:
        printf("Got ip\n");
        printf("stack space is %d\n",uxTaskGetStackHighWaterMark(NULL));
        break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
        printf("dissconnected...\n");
        break;

        default:
        break;

    }
    return ESP_OK;
}

void wifiInit()
{
  ESP_ERROR_CHECK(nvs_flash_init());
  tcpip_adapter_init();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

  wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  wifi_config_t wifi_config =
      {
          .sta = {
              .ssid = SSID,
              .password = PASSWORD
              }};
  esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
  ESP_ERROR_CHECK(esp_wifi_start());
}

void onConnected(void *params)
{
    while(true)
    {
        if(xSemaphoreTake(onConnectionHandler, 10*1000 / portTICK_PERIOD_MS));
        {
            //do something
            xSemaphoreTake(onConnectionHandler, portMAX_DELAY));
        }
        else
        {
            esp_restart();
        }
    }
}

void app_main(void)
{
    onConnectionHandler = xSemaphoreCreateBinary();
    wifiInit();
    xTaskCreate(&onConnected,"On connected", 1024*4, NULL, 5, NULL);
}
