#ifndef _GPIO_H
#define _GPIO_H


typedef enum
{
    LO_TO_HI = 1,
    HI_TO_LO,
    ANY_EDGE
} interrupt_type_edge_t;

typedef void (*input_callback_t)(int);

//Input interrupt
void input_io_creat(gpio_num_t gpio_num, interrupt_type_edge_t type);
int input_io_get_level(gpio_num_t gpio_num);
void input_set_callback(void *cb);


void output_io_create(gpio_num_t gpio_num);
void output_io_set_level(gpio_num_t gpio_num, bool state);
void output_io_toggle(gpio_num_t gpio_num);









#endif /* _GPIO_H */