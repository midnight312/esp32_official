#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "gpio.h"

static const char *TAG = "Button";
static uint64_t _start, _stop, _press;


#define TIM_DELAY 1000

static uint8_t s_switch_state = 0;

input_callback_t input_callback = NULL;

extern void IRAM_ATTR gpio_input_handler(void *args)
{
    int pinNumber = (int)args;
    //uint32_t rtc = xTaskGetTickCountFromISR();
    input_callback(pinNumber);  
}

void input_io_creat(gpio_num_t gpio_num, interrupt_type_edge_t type)
{
    gpio_pad_select_gpio(gpio_num);
    gpio_set_direction(gpio_num, GPIO_MODE_INPUT);
    gpio_set_pull_mode(gpio_num, GPIO_PULLDOWN_ONLY);
    gpio_set_intr_type(gpio_num, type);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(gpio_num, gpio_input_handler, (void *)gpio_num);
}

int input_io_get_level(gpio_num_t gpio_num)
{
    return gpio_get_level(gpio_num);
}

void input_set_callback(void *cb)
{
    input_callback = cb;
}

void output_io_create(gpio_num_t gpio_num)
{
    gpio_pad_select_gpio(gpio_num);
    gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT);
}
void output_io_set_level(gpio_num_t gpio_num, bool state)
{
    gpio_set_level(gpio_num, state);
}
void output_io_toggle(gpio_num_t gpio_num)
{
    int old_level = gpio_get_level(gpio_num);
    gpio_set_level(gpio_num, 1-old_level);

}