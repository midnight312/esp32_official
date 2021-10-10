/* ADC1 Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"

uint32_t hall_sens_read();

void app_main(void)
{
    //adc1_config_width(ADC_WIDTH_BIT_12);
    //adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_0);
    while(true)
    {
        //int val = adc1_get_raw(ADC1_CHANNEL_3);
        printf("Hall sens: %d\n",(uint32_t)hall_sens_read());
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    /*DCA
    dac_output_enable();
    dac_out_voltage();
    */
}
