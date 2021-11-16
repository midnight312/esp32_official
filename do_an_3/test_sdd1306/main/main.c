#include <stdio.h>
#include "fonts.h"
#include "i2c.h"
#include "ssd1306.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "user_uart.h"
#include "driver/uart.h"

extern const int RX_BUF_SIZE = 1024;

void display1(void);
void display2(void);

static void tx_task(void *arg)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
        sendDataUART(TX_TASK_TAG, "Hello world");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);
    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_RATE_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
            //ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
    }
    free(data);
}
void display1(void)
{
    ssd1306_clear(0);
    ssd1306_select_font(0, 1);
    ssd1306_draw_string(0, 10, 0, "task1 hien thi" , 2, 0);;
    ssd1306_refresh(0, true);
    vTaskDelay( 2000 / portTICK_PERIOD_MS);
    display2();
}

void display2(void)
{
    ssd1306_clear(0);
    ssd1306_select_font(0, 1);
    ssd1306_draw_string(0, 10, 0, "task2 hien " , 2, 0);
    ssd1306_refresh(0, true);
    vTaskDelay( 2000 / portTICK_PERIOD_MS);
    display1();
}

void app_main(void)
{
    
    init_UART();
    //xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
    ssd1306_init(0, 19, 22);
    //xTaskCreate(&display1, "uart_tx_task", 1024*2, NULL, 5 , NULL);
    //xTaskCreate(&display2, "uart_tx_task", 1024*2, NULL, 4 , NULL);
    
    display1();

    

}
