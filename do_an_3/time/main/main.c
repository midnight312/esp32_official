#include <stdio.h>
#include <stdlib.h>
#include "esp_wifi.h"
#include <time.h>
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "esp_log.h"

#define TAG "NTP_TIME"

void printf_time(long time, const char *message)
{
    setenv("TZ","SGT-7",1);
    tzset();
    struct tm *timeinfo = localtime(&time);
    char buffer[50];
    strftime(buffer,sizeof(buffer),"%c",timeinfo);
    ESP_LOGI(TAG,"MESSAGE %s : %s",message,buffer);

}

void on_got_time(struct timeval *tv)
{
    printf("sec %ld\n",tv->tv_sec);
    printf_time(tv->tv_sec,"time_at_callback");
    example_disconnect();
}


void app_main(void)
{
    nvs_flash_init();
    tcpip_adapter_init();
    esp_event_loop_create_default();
    example_connect();

    sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
    sntp_set_time_sync_notification_cb(on_got_time);

}
