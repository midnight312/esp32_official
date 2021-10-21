#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_http_client.h"
#include "esp_log.h"
#include "fetch.h"
#include "cJSON.h"

char *tag = "CONNECTION";

char *buffer = NULL;
int Index = 0;


esp_err_t clientEventHandler(esp_http_client_event_t *evt)
{
    switch(evt->event_id)
    {
        struct FetchParams *fetchParams = (struct FetchParams *)evt->user_data;
        case HTTP_EVENT_ON_DATA:
        ESP_LOGI(tag, "HTTP_EVENT_ON_DATA Len = %d",evt->data_len);
        printf("%.*s\n",evt->data_len, (char *)evt->data);
        if(buffer == NULL)
        {
            buffer = (char *)malloc(evt->data_len);
        }
        else
        {
            buffer = (char *)realloc(buffer, evt->data_len + Index);
        }
        memcpy(&buffer[Index],evt->data,evt->data_len);
        Index += evt->data_len;
        break;

        case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(tag, "HTTP_EVENT_ON_FINISH");
        buffer = (char *)realloc(buffer,Index + 1);
        memcpy(&buffer[Index], "\0",1);
        ESP_LOGI(tag, "%s",buffer);
        //onGotData(buffer);
        free(buffer);
        Index = 0;
        break;

        default:
        break;
    }
    return ESP_OK;
}


void fetch(char* url, struct FetchParams *fetchParams)
{
    esp_http_client_config_t clientConfig = {
        .url = url,
        .event_handler = clientEventHandler,
        .user_data = fetchParams
    };

    esp_http_client_handle_t client = esp_http_client_init(&clientConfig);
    esp_err_t err = esp_http_client_perform(client);
    if(err == ESP_OK)
    {
        ESP_LOGI(tag, "HTTP Get Status %d, content length = %d\n",esp_http_client_get_status_code(client),esp_http_client_get_content_length(client));

    }
    else
    {
        ESP_LOGE(tag, "HTTP GET Request Fail : %s",esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);

}