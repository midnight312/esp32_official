#include <stdio.h>
#include <esp_wifi.h>
#include <esp_event_loop.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <nvs_flash.h>



#define MAX_APs 20

static esp_err_t event_handler(void * ctx, system_event_t *event)
{

    return ESP_OK;
}

void wifiInit()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

}

static char *getAuthModeName(wifi_auth_mode_t auth_mode)
{
    char* name[] = {"OPEN","WEP","WPA PSK","WPA2 PSK","WPA WPA2 PSK","MAX"};
    return name[auth_mode];
}





void app_main(void)
{
    wifiInit();

    wifi_scan_config_t scan_config = {
        .bssid = 0,
        .ssid = 0,
        .channel = 0,
        .show_hidden = true
    };
    esp_wifi_scan_start(&scan_config, true);

    wifi_ap_record_t wifi_record[MAX_APs];

    uint16_t maxRecod = MAX_APs;
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&maxRecod, wifi_record));

    printf("Find %d access points\n\n",maxRecod);
    printf("            SSID                |  Channel  |  RSSI  |  Auth Mode  \n");
    
    printf("-------------------------------------------------------------------\n");
    for(int i = 0; i < maxRecod; i++)
    {
        printf(" %30s | %9d | %6d | %12s\n",(char *)wifi_record[i].ssid,wifi_record[i].primary,wifi_record[i].rssi,getAuthModeName(wifi_record[i].authmode));
        printf("-------------------------------------------------------------------\n");
    }
}
