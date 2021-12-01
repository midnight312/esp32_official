/* Mirf Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "mirf.h"

typedef union {
  uint8_t value[1];
  unsigned long now_time;
} MYDATA_t;


MYDATA_t mydata;


#if CONFIG_RECEIVER
void receiver(void *pvParameters)
{
	NRF24_t dev;

	ESP_LOGI(pcTaskGetTaskName(0), "Start");
	ESP_LOGI(pcTaskGetTaskName(0), "CONFIG_CE_GPIO=%d",CONFIG_CE_GPIO);
	ESP_LOGI(pcTaskGetTaskName(0), "CONFIG_CSN_GPIO=%d",CONFIG_CSN_GPIO);
	spi_master_init(&dev, CONFIG_CE_GPIO, CONFIG_CSN_GPIO, CONFIG_MISO_GPIO, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO);

	Nrf24_setRADDR(&dev, (uint8_t *)"FGHIJ");
	uint8_t payload = sizeof(mydata.value);
	uint8_t channel = 90;
	Nrf24_config(&dev, channel, payload);
	Nrf24_printDetails(&dev);
	ESP_LOGI(pcTaskGetTaskName(0), "Listening...");
	uint8_t tbuff[1];
	while(1) {
		if (Nrf24_dataReady(&dev)) { //When the program is received, the received data is output from the serial port
			Nrf24_getData(&dev, tbuff);
			ESP_LOGI(pcTaskGetTaskName(0), "Got data:%c",tbuff[0]);
		}
		vTaskDelay(1);
	}
}
#endif


#if CONFIG_TRANSMITTER
void transmitter(void *pvParameters)
{
	NRF24_t dev;
	
	
	ESP_LOGI(pcTaskGetTaskName(0), "Start");
	ESP_LOGI(pcTaskGetTaskName(0), "CONFIG_CE_GPIO=%d",CONFIG_CE_GPIO);
	ESP_LOGI(pcTaskGetTaskName(0), "CONFIG_CSN_GPIO=%d",CONFIG_CSN_GPIO);
	spi_master_init(&dev, CONFIG_CE_GPIO, CONFIG_CSN_GPIO, CONFIG_MISO_GPIO, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO);

	Nrf24_setRADDR(&dev, (uint8_t *)"ABCDE");
	uint8_t payload = 7;
	uint8_t channel = 90;
	Nrf24_config(&dev, channel, payload);
	Nrf24_printDetails(&dev);

	while(1) {
		uint8_t value[7];
		value[0] = 'a';
		value[1] = 'b';
		value[2] = 'c';
		value[3] = 'd';
		value[4] = 'e';
		value[5] = 'f';
		value[6] = 0;
		mydata.now_time = xTaskGetTickCount();
		Nrf24_setTADDR(&dev, (uint8_t *)"BCDEF");			//Set the receiver address
		Nrf24_send(&dev, value);				   //Send instructions, send random number value
		vTaskDelay(1);
		ESP_LOGI(pcTaskGetTaskName(0), "Wait for sending.....");
		if (Nrf24_isSend(&dev)) {
			ESP_LOGI(pcTaskGetTaskName(0),"Send success:%s", value);
		} else {
			ESP_LOGI(pcTaskGetTaskName(0),"Send fail:");
		}
		vTaskDelay(1000/portTICK_PERIOD_MS);
	}
}


#endif

	
#define tag "MIRF"

void app_main(void)
{
#if CONFIG_RECEIVER
	// Create Task
	xTaskCreate(receiver, "RECV", 1024*2, NULL, 2, NULL);
#endif

#if CONFIG_TRANSMITTER
	// Create Task
	xTaskCreate(transmitter, "TRANS", 1024*2, NULL, 2, NULL);
#endif

}
