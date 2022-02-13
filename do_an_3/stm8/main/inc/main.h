#ifndef _MAIN_H
#define _MAIN_H

#include <stdio.h>
#include "stm8s.h"
#include "stm8s_clk.h"
#include "stm8s_gpio.h"
#include "stm8s_tim2.h"
#include "stm8s_adc1.h"
#include "stm8s_uart1.h"
#include "stm8s_spi.h"
#include "rf24l01.h"


#define MACHINE_NUMBER  0x00                    //ID number for each collection module

#define BULB            0x01
#define FAN             0x02
#define PUMB_IN         0x04
#define PUMB_OUT        0x08
#define MAN_CHE         0x10

//UART1 PD5-TX PD6-RX
#define baurateUART     112500

//so lan do cua cam bien
#define ACCURARY 2

enum data
{
    data_humidity = 0,
    data_light,
    data_temperature
};

//RF24L01
typedef struct _data_to_send {
  uint32_t add;
  uint32_t sub;
  uint32_t mult;
  uint32_t div;
} data_to_send;


typedef struct _data_received {
  uint32_t op1;
  uint32_t op2;
} data_received;

void delay_us(uint16_t x);
void delay_ms(uint16_t x);



#endif /* _MAIN_H */