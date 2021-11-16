#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "fetch.h"
#include "connect.h"
#include "cJSON.h"
#include <string.h>


static char *TAG = "NOTIFI MAIN";

xSemaphoreHandle connectionSemaphore;

//#define NUMBER "0399005619"



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

void createBody(char *number, char *message, char *out)
{
  sprintf(out,
          "{"
          "  \"messages\": ["
          "      {"
          "      "
          "          \"content\": \"%s\","
          "          \"destination_number\": \"%s\","
          "          \"format\": \"SMS\""
          "      }"
          "  ]"
          "}",
          message, number);
}

void onConnected(void *para)
{
    while(true)
    {
        if(xSemaphoreTake(connectionSemaphore, 10000 / portTICK_RATE_MS))
        {
            ESP_LOGI(TAG, "Processing.\n");
            struct FetchParams fetchParams;
            fetchParams.OnGotData = OnGotData;
            fetchParams.body = NULL;
            fetchParams.headCount = 0;
            fetchParams.method = Get;

            fetch("http://quotes.rest/qod",&fetchParams);
            if(fetchParams.status == 200)
            {
                //do something
                struct FetchParams temperatureStruct;
                temperatureStruct.OnGotData = NULL;
                temperatureStruct.method = Get;
                temperatureStruct.body = NULL;

                Header headerAPIkey = {
                    .key = "api_key",
                    .value = "N3G729V2LY0JV1E4"
                };
                Header headerField1 = {
                    .key = "field1",
                    .value = "25"
                };
                Header headerField2 = {
                    .key = "field2",
                    .value = "10"
                };
                temperatureStruct.header[0] = headerAPIkey;
                temperatureStruct.header[1] = headerField1;
                temperatureStruct.header[2] = headerField2;
                temperatureStruct.headCount = 0;

                //char buffer[1024];
                //createBody(NUMBER, fetchParams.message, buffer);
                temperatureStruct.body = NULL;
                
                fetch("GET https://api.thingspeak.com/update?api_key=N3G729V2LY0JV1E4&field1=25&field2=25\n\n",&temperatureStruct);
            }
            ESP_LOGI(TAG, "Done!\n");
            esp_wifi_disconnect();
            xSemaphoreTake(connectionSemaphore, portMAX_DELAY);
        }
        else
        {
            ESP_LOGI(TAG, "Fail to connect. Retry in");
            for(int i = 0; i < 5; i++)
            {
                ESP_LOGI(TAG, "%d",5 - i);
                vTaskDelay(1000 / portTICK_RATE_MS);
            }
            esp_restart();
        }
    }
}

void app_main(void)
{
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    connectionSemaphore = xSemaphoreCreateBinary();
    wifiInit();
    xTaskCreate(&onConnected, "hander comms", 1024*3, NULL, 5, NULL);

}
