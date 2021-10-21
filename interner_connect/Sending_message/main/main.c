#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "fetch.h"
#include "connect.h"

#define TAG "DATA"

char* number = "";

xSemaphoreHandle connectionSemaphore;

void createBody(char *number, char *message, char *out)
{
  sprintf(out,
    "{
    \message\:

  }")
}

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

void OnConnected(void *para)
{
  
  while (true)
  {
    if (xSemaphoreTake(connectionSemaphore, 10000 / portTICK_RATE_MS))
    {
      ESP_LOGI(TAG, "Processing");
      struct FetchParms fetchParams;
      fetchParams.OnGotData = OnGotData;
      fetchParams.body = NULL;
      fetchParams.headerCount = 0;
      fetchParams.method = GET;
      
      fetch("http://quotes.rest/qod", &fetchParams);
      if(fetchParams.status == 200)
      {
        //send sms
        struct FetchParms smsStruct;
        smsStruct.OnGotData = NULL;
        smsStruct.method = POST;
        Header headerContentType1 = {
          .key = "",
          .value = ""
        };
        Header headerContentType2 = {
          .key = "",
          .value = ""
        };
        Header headerContentType3 = {
          .key = "",
          .value = ""
        };
        smsStruct.header[0] = headerContentType1;
        smsStruct.header[1] = headerContentType2;
        smsStruct.header[2] = headerContentType3;
        smsStruct.headerCount = 3;
        char buffer[1024];
        createBody(number, fetchParams.message, buffer);
        smsStruct.body = buffer;
        fetch("",&smsStruct);

      }

      ESP_LOGI(TAG, "%s", fetchParams.message);

      ESP_LOGI(TAG, "Done!");
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

void app_main()
{
  esp_log_level_set(TAG, ESP_LOG_DEBUG);
  connectionSemaphore = xSemaphoreCreateBinary();
  wifiInit();
  xTaskCreate(&OnConnected, "handel comms", 1024 * 3, NULL, 5, NULL);
}
