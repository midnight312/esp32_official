#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"
#include "user_uart.h"

#define TAG "UART"


#define TXD_PIN_UART_1 (GPIO_NUM_4)
#define RXD_PIN_UART_1 (GPIO_NUM_5)

#define TXD_PIN_UART_2 (GPIO_NUM_17)
#define RXD_PIN_UART_2 (GPIO_NUM_16)

#define RX_BUF_SIZE 1024
#define TX_BUF_SIZE 1024

/*
QueueHandle_t uart_queue;

void uart_event_task(void *params)  
{
  uart_event_t uart_event;
  uint8_t *received_buffer = malloc(RX_BUF_SIZE);
  size_t datalen;
  while (true)
  {
    if (xQueueReceive(uart_queue, &uart_event, portMAX_DELAY))
    {
      switch (uart_event.type)
      {
      case UART_DATA:
        ESP_LOGI(TAG, "UART_DATA");
        uart_read_bytes(UART_NUM_1, received_buffer, uart_event.size, portMAX_DELAY);
        printf("received: %.*s\n", uart_event.size, received_buffer);
        break;
      case UART_BREAK:
        ESP_LOGI(TAG, "UART_BREAK");
        break;
      case UART_BUFFER_FULL:
        ESP_LOGI(TAG, "UART_BUFFER_FULL");
        break;
      case UART_FIFO_OVF:
        ESP_LOGI(TAG, "UART_FIFO_OVF");
        uart_flush_input(UART_NUM_1);
        xQueueReset(uart_queue);
        break;
      case UART_FRAME_ERR:
        ESP_LOGI(TAG, "UART_FRAME_ERR");
        break;
      case UART_PARITY_ERR:
        ESP_LOGI(TAG, "UART_PARITY_ERR");
        break;
      case UART_DATA_BREAK:
        ESP_LOGI(TAG, "UART_DATA_BREAK");
        break;
      case UART_PATTERN_DET:
        ESP_LOGI(TAG, "UART_PATTERN_DET");
        break;
      }
    }
  }
}
*/
void init_UART(void) {
    const uart_config_t uart_config_1 = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    const uart_config_t uart_config_2 = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, 1024 * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config_1);
    uart_set_pin(UART_NUM_1, TXD_PIN_UART_1, RXD_PIN_UART_1, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    uart_driver_install(UART_NUM_2, 1024 * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_2, &uart_config_2);
    uart_set_pin(UART_NUM_2, TXD_PIN_UART_2, RXD_PIN_UART_2, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

}




