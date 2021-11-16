#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_http_client.h"
#include "esp_log.h"
#include "fetch.h"
#include "connect.h"

static char *TAG = "Http Client";

char *buffer = NULL;
int indexBuffer = 0;

esp_err_t clientEventHandler(esp_http_client_event_t *evt)
{
    struct FetchParams *fetchParams = (struct FetParams *)evt->user_data;   
    switch(evt->event_id)
    {
        case HTTP_EVENT_ON_DATA:
        ESP_LOGI(TAG, "HTTP EVENT ON DATA LENGTH = %d", evt->data_len);
        printf("%.*s\n",evt->data_len,(char *)evt->data);
        if(buffer == NULL)
        {
            buffer = (char *)malloc(evt->data_len);
        }
        else{
            buffer = (char *)realloc(buffer, evt->data_len + indexBuffer);
        }
        memcpy(&buffer[indexBuffer], evt->data, evt->data_len);
        indexBuffer += evt->data_len;
        break;

        case HTTP_EVENT_ON_FINISH:
        ESP_LOGI(TAG, "HTTP EVENT ON FINISH\n");
        buffer = (char *)realloc(buffer, indexBuffer + 1);
        memcpy(&buffer[indexBuffer],"\0", 1);
        if(fetchParams->OnGotData != NULL)
        {
            fetchParams->OnGotData(buffer, fetchParams->message);
        }
        free(buffer);
        buffer = NULL;
        indexBuffer = 0;    
        break;

        default:
        break;
    }
    return ESP_OK;
}


void fetch(char *url, struct FetchParams *fetchParams)
{
    esp_http_client_config_t clientConfig = {
        .url = url,
        .event_handler = clientEventHandler,
        .user_data = fetchParams
    };
    esp_http_client_handle_t client = esp_http_client_init(&clientConfig);
    if(fetchParams->method == Post)
    {
        esp_http_client_set_method(client, HTTP_METHOD_POST);
    }
    for(int i = 0; i<fetchParams->headCount; i++)
    {
        esp_http_client_set_header(client, fetchParams->header[i].key, fetchParams->header[i].value);
    }
    if(fetchParams->body != NULL)
    {
        esp_http_client_set_post_field(client, fetchParams->body, strlen(fetchParams->body));
    }
    esp_err_t err = esp_http_client_perform(client);
    fetchParams->status = esp_http_client_get_status_code(client);
    if(err == ESP_OK)
    {
        ESP_LOGI(TAG, "HTTP Get Status %d, length %d",
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    }
    else
    {
        ESP_LOGE(TAG, "ESP Request Fail: %s",esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}
