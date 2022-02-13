#include "main.h"



uint8_t control = 0x00;
static uint8_t old_control;
volatile uint16_t data[3];
volatile static uint8_t data_send[6];
static uint8_t *ptr = NULL;

uint8_t status_machine;

uint8_t index = 0;
uint8_t i = 0;

//rf24l01
extern uint8_t mutex;
data_to_send to_send;
data_received received;

void assert_failed(uint8_t* file, uint32_t line){}


//ADC for cam bien do am dat
void adc_CH4_conf(void)
{
    //Delelte init all ADC config
    ADC1_DeInit();
    //Config gpio for channel ain 4 pin PD3
    GPIO_Init(GPIOD, GPIO_PIN_3, GPIO_MODE_IN_FL_NO_IT);
    //Config ADC channel ain 4 PD3
    ADC1_Init(ADC1_CONVERSIONMODE_SINGLE,\
              ADC1_CHANNEL_4, ADC1_PRESSEL_FCPU_D2,\
              ADC1_EXTTRIG_TIM, DISABLE, ADC1_ALIGN_RIGHT,\
              ADC1_SCHMITTTRIG_CHANNEL4, DISABLE); 
}

//ADC for cam bien anh sag
void adc_CH3_conf(void)
{
    //Delelte init all ADC config
    ADC1_DeInit();
    //Config gpio for channel ain 3 pin PD2
    GPIO_Init(GPIOD, GPIO_PIN_2, GPIO_MODE_IN_FL_NO_IT);
    //Config ADC channel ain 3 PD2
    ADC1_Init(ADC1_CONVERSIONMODE_SINGLE,\
              ADC1_CHANNEL_3, ADC1_PRESSEL_FCPU_D2,\
              ADC1_EXTTRIG_TIM, DISABLE, ADC1_ALIGN_RIGHT,\
              ADC1_SCHMITTTRIG_CHANNEL3, DISABLE); 
}

//ADC for cam bien nhiet do
void adc_CH2_conf(void)
{
    //Delelte init all ADC config
    ADC1_DeInit();
    //Config gpio for channel ain 1 pin PD4
    GPIO_Init(GPIOC, GPIO_PIN_4, GPIO_MODE_IN_FL_NO_IT);
    //Config ADC channel ain 1 PD4
    ADC1_Init(ADC1_CONVERSIONMODE_SINGLE,\
              ADC1_CHANNEL_2, ADC1_PRESSEL_FCPU_D2,\
              ADC1_EXTTRIG_TIM, DISABLE, ADC1_ALIGN_RIGHT,\
              ADC1_SCHMITTTRIG_CHANNEL2, DISABLE); 
}

//Control machine
void init_GPIO_machine()
{
    /* Init GPIO for pin control machine */
    //Bulb
    GPIO_Init(GPIOD, GPIO_PIN_1, GPIO_MODE_OUT_PP_LOW_SLOW);
    //Fan
    GPIO_Init(GPIOC, GPIO_PIN_6, GPIO_MODE_OUT_PP_LOW_SLOW);
    //Pump in
    GPIO_Init(GPIOB, GPIO_PIN_4, GPIO_MODE_OUT_PP_LOW_SLOW);
    //Pumb out
    GPIO_Init(GPIOC, GPIO_PIN_5, GPIO_MODE_OUT_PP_LOW_SLOW);
    //Man che
    GPIO_Init(GPIOA, GPIO_PIN_3, GPIO_MODE_OUT_PP_LOW_SLOW);
    
   //Off all
    GPIO_WriteHigh(GPIOD, GPIO_PIN_1);
    GPIO_WriteHigh(GPIOC, GPIO_PIN_6);
    GPIO_WriteHigh(GPIOB, GPIO_PIN_4);
    GPIO_WriteHigh(GPIOC, GPIO_PIN_5);
    GPIO_WriteHigh(GPIOA, GPIO_PIN_3);
}
void make_machine()
{
  if((control & BULB) != 0)
  {
    GPIO_WriteLow(GPIOD, GPIO_PIN_1);
  }
  else
  {
    GPIO_WriteHigh(GPIOD, GPIO_PIN_1);
  }
  
  if((control & FAN) != 0)
  {
    GPIO_WriteLow(GPIOC, GPIO_PIN_6);
  }
  else
  {
    GPIO_WriteHigh(GPIOC, GPIO_PIN_6);
  }
  
  if((control & PUMB_IN) != 0)
  {
    GPIO_WriteLow(GPIOB, GPIO_PIN_4);
  }
  else
  {
    GPIO_WriteHigh(GPIOB, GPIO_PIN_4);
  }
  
  if((control & PUMB_OUT) != 0)
  {
    GPIO_WriteLow(GPIOC, GPIO_PIN_5);
  }
  else
  {
    GPIO_WriteHigh(GPIOC, GPIO_PIN_5);
  }
  
  if((control & MAN_CHE) != 0)
  {
    GPIO_WriteLow(GPIOA, GPIO_PIN_3);
  }
  else
  {
    GPIO_WriteHigh(GPIOA, GPIO_PIN_3);
  }
  
}

void delay_us(uint16_t x)
{
     while(x--)
     {
           nop();
           nop();
           nop();
           nop();
           nop();
           nop();
           nop();
           nop();
           nop();
     }
}
void delay_ms(uint16_t x)
{
     while(x--)
     {
        delay_us(1000);
     }
}

#if 0
void rf24l01_init(void)
{   
    mutex = 0;
    RF24L01_init();
    uint8_t rx_addr[5] = {0x41, 0x42, 0x43, 0x44, 0x45};
    uint8_t tx_addr[5] = {0x42, 0x43, 0x44, 0x45, 0x46};
    RF24L01_setup(tx_addr, rx_addr, 90);
    
    //IRQ
    GPIO_Init(GPIOB, GPIO_PIN_4, GPIO_MODE_IN_FL_IT);
    EXTI_SetExtIntSensitivity(EXTI_PORT_GPIOB, EXTI_SENSITIVITY_FALL_ONLY);
}

void rf24l01_wait_data(void)
{  
    old_control = control;
    RF24L01_set_mode_RX();
    uint8_t temp[2];
    if (mutex == 1) 
    {
      RF24L01_read_payload((uint8_t *)temp, 2);
      if(temp[0] == MACHINE_NUMBER)
      {
        control = temp[1];
      }
      if(old_control != control)
      {
         old_control = control;
         //Bulb
         if((control & BULB) == 1)
         {
             GPIO_WriteLow(GPIOD, GPIO_PIN_1);
         }
         else
         {
             GPIO_WriteHigh(GPIOD, GPIO_PIN_1);
         }
         //Fan
         if((control & FAN) == 1)
         {
             GPIO_WriteLow(GPIOD, GPIO_PIN_4);
         }
         else
         {
             GPIO_WriteHigh(GPIOD, GPIO_PIN_4);
         }
         //Pumb in
         if((control & PUMB_IN) == 1)
         {
             GPIO_WriteLow(GPIOB, GPIO_PIN_5);
         }
         else
         {
             GPIO_WriteHigh(GPIOB, GPIO_PIN_5);
         }
         //Pumb out
         if((control & PUMB_OUT) == 1)
         {
             GPIO_WriteLow(GPIOD, GPIO_PIN_6);
         }
         else
         {
             GPIO_WriteHigh(GPIOD, GPIO_PIN_6);
         }
         //Man che
         if((control & MAN_CHE) == 1)
         {
             GPIO_WriteLow(GPIOA, GPIO_PIN_3);
         }
         else
         {
             GPIO_WriteHigh(GPIOA, GPIO_PIN_3);
         }
      }
      mutex = 0;
      RF24L01_set_mode_TX();
      RF24L01_write_payload((uint8_t *)data_send, 7);
    }
    else if(mutex == 2)
    {
      //transmit false
      
    }
}

#endif

void cv_data_to_8(void)
{
  ptr = (uint8_t *)data;
  for(int i = 0; i<6; i++)
  {
    data_send[i] = (uint8_t)*(ptr + i);
  }
}

void UART1_SendString()
{
  cv_data_to_8();
  for(int i = 0; i < 6; i++)
  {
    UART1_SendData8(data_send[i]);
    delay_ms(1);
  }

}
void main() 
{
    disableInterrupts();
    CLK_HSICmd(ENABLE);
    CLK_HSECmd(DISABLE);
    CLK_LSICmd(DISABLE);
    CLK_HSIPrescalerConfig(0);
    init_GPIO_machine();
    //rf24l01_init();
    UART1_DeInit();
    UART1_Init(115200, UART1_WORDLENGTH_8D, UART1_STOPBITS_1, UART1_PARITY_NO, UART1_SYNCMODE_CLOCK_DISABLE, UART1_MODE_TXRX_ENABLE);
    UART1_ITConfig(UART1_IT_RXNE_OR, ENABLE);
    UART1_Cmd(ENABLE);
    
    enableInterrupts();
    
    while(1)
    {
        //rf24l01_wait_data();
           
        make_machine();
      
        memset(data, NULL, 6);
        //cam bien do am dat
        uint32_t adc_temp = 0;
        adc_CH4_conf();
        
        ADC1_StartConversion();  
        for( i = 0; i<ACCURARY; i++) 
        {
          while(!(ADC1_GetFlagStatus(ADC1_FLAG_EOC)&0X80));
          {
              adc_temp += ADC1_GetConversionValue();
          }
        }
        data[data_humidity] = (uint16_t)(adc_temp/ACCURARY);
        adc_temp = 0;
        
        //cam bien as
        adc_CH3_conf();
        ADC1_StartConversion();       
        for( i = 0; i<ACCURARY; i++)
        {
          while(!(ADC1_GetFlagStatus(ADC1_FLAG_EOC)&0X80));
          {
              adc_temp += ADC1_GetConversionValue();
          }
        }
        data[data_light] = (uint16_t)(adc_temp/ACCURARY);
        adc_temp = 0;
        //cam bien nhiet do
        adc_CH2_conf();
        ADC1_StartConversion();         
        for( i = 0; i<ACCURARY; i++)
        {
          while(!(ADC1_GetFlagStatus(ADC1_FLAG_EOC)&0X80));
          {
              adc_temp += ADC1_GetConversionValue();
          }
        }
        data[data_temperature] = (uint16_t)(adc_temp/ACCURARY);
        adc_temp = 0;
        
        UART1_SendString();   
        delay_ms(3000);
        
    }     
}
