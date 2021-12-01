#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "esp_log.h"
//#include "nvs_flash.h"
#include "cJSON.h"
#include "fetch.h"
#include "connect.h"
#include "ssd1306.h"
#include "fonts.h"


#define TAG "DATA"

xSemaphoreHandle connectionSemaphore;

char control_machine = 0x00;

void OnGotData(char *incomingBuffer, char* output)
{
    cJSON *payload = cJSON_Parse(incomingBuffer);
    cJSON *contents = cJSON_GetObjectItem(payload, "contents");
    cJSON *quotes = cJSON_GetObjectItem(contents, "quotes");
    cJSON *quotesElement;
    cJSON_ArrayForEach(quotesElement, quotes)
    {
        cJSON *quote = cJSON_GetObjectItem(quotesElement, "quote");
        ESP_LOGI(TAG,"%s",quote->valuestring);
        strcpy(output, quote->valuestring);
    }
    cJSON_Delete(payload);
}

void displaySSD1306(char *str)
{
		//ssd1306_draw_rectangle(0, 10, 30, 20, 20, 1);
		//ssd1306_select_font(0, 0);
		//ssd1306_draw_string(0, 0, 0, "glcd_5x7_font_info", 1, 0);
		ssd1306_select_font(0, 1);
		ssd1306_draw_string(0, 20, 30, str, 2, 0);
		ssd1306_refresh(0, true);

}

void OnConnected(void *para)
{
  
  while (true)
  {
    if (xSemaphoreTake(connectionSemaphore, 10000 / portTICK_RATE_MS))
    {
      ESP_LOGI(TAG, "Processing");
      struct FetchParms fetchParams;
      fetchParams.OnGotData = OnGotData;
      
      fetch("http://quotes.rest/qod", &fetchParams);
      ESP_LOGI(TAG, "%s", fetchParams.message);
      ESP_LOGI(TAG, "Done!");
      displaySSD1306("Done");
      esp_wifi_disconnect();
      xSemaphoreTake(connectionSemaphore, portMAX_DELAY);
    }
    else
    {
      ESP_LOGE(TAG, "Failed to connect. Retry in");
      for (int i = 0; i < 5; i++)
      {
        ESP_LOGE(TAG, "...%d", i);
        vTaskDelay(1000 / portTICK_RATE_MS);
      }
      esp_restart();
    }
  }
}



void init_gpio(void)
{
  
}

void app_main()
{
  esp_log_level_set(TAG, ESP_LOG_DEBUG);
  connectionSemaphore = xSemaphoreCreateBinary();
  ssd1306_init(0, 19, 22);
  wifiInit();
  xTaskCreate(&OnConnected, "handel comms", 1024 * 3, NULL, 5, NULL);
}