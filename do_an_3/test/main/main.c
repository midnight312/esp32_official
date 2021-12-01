#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "cJSON.h"
#include "fetch.h"
#include "connect.h"
#include "ssd1306.h"
#include "fonts.h"
#include "gpio.h"



#define TAG "DATA"


#define BUTTON_BACK 12
#define BUTTON_HOME 15
#define BUTTON_NEXT 13


#define BUTTON_BACK_PRESS       (1<<0)
#define BUTTON_BACK_LONG_PRESS  (1<<1)
#define BUTTON_HOME_PRESS       (1<<2)
#define BUTTON_HOME_LONG_PRESS  (1<<3)
#define BUTTON_NEXT_PRESS       (1<<4)

#define DISPLAY_HOME            (1<<0)
#define DISPLAY_MENU_1          (1<<1)
#define DISPLAY_MENU_2          (1<<2)
#define DISPLAY_GARDEN_1        (1<<3)
#define DISPLAY_GARDEN_2        (1<<4)

#define NUM_TIMERS 3


xSemaphoreHandle connectionSemaphore;
EventGroupHandle_t xButtonEventGroup;
EventGroupHandle_t xDisplayEventGroup;

xQueueHandle interputQueueButtonBack;
xQueueHandle interputQueueButtonHome;
xQueueHandle interputQueueButtonNext;

TimerHandle_t xTimers[ NUM_TIMERS ];

typedef struct data_gpio_iterrupt
{
  gpio_num_t gpio_num;
  uint64_t press_tick;
} data_gpio_iterrupt_t;

static bool status_button[2];

void IRAM_ATTR gpio_input_handler(void *args);


void OnGotData(char *incomingBuffer, char* output)
{
    cJSON *payload = cJSON_Parse(incomingBuffer);
    cJSON *contents = cJSON_GetObjectItem(payload, "contents");
    cJSON *quotes = cJSON_GetObjectItem(contents, "quotes");
    cJSON *quotesElement;
    cJSON_ArrayForEach(quotesElement, quotes)
    {
        cJSON *quote = cJSON_GetObjectItem(quotesElement, "quote");
        ESP_LOGI(TAG,"%s",quote->valuestring);
        strcpy(output, quote->valuestring);
    }
    cJSON_Delete(payload);
}

void displaySSD1306(char *str)
{
		//ssd1306_draw_rectangle(0, 10, 30, 20, 20, 1);
		//ssd1306_select_font(0, 0);
		//ssd1306_draw_string(0, 0, 0, "glcd_5x7_font_info", 1, 0);
}

void OnConnected(void *para)
{
  
  while (true)
  {
    if (xSemaphoreTake(connectionSemaphore, 10000 / portTICK_RATE_MS))
    {
      ESP_LOGI(TAG, "Processing");
      struct FetchParms fetchParams;
      fetchParams.OnGotData = OnGotData;
      
      fetch("http://quotes.rest/qod", &fetchParams);
      ESP_LOGI(TAG, "%s", fetchParams.message);
      ESP_LOGI(TAG, "Done!");
      displaySSD1306("Done");
      esp_wifi_disconnect();
      xSemaphoreTake(connectionSemaphore, portMAX_DELAY);
    }
    else
    {
      ESP_LOGE(TAG, "Failed to connect. Retry in");
      for (int i = 0; i < 5; i++)
      {
        ESP_LOGE(TAG, "...%d", i);
        vTaskDelay(1000 / portTICK_RATE_MS);
      }
      esp_restart();
    }
  }
}




void input_event_callback(int gpio_num)
{
  int num = gpio_num;
  /* BUTTON HOME */
  if(num == BUTTON_HOME)
  {
    xQueueSendFromISR(interputQueueButtonHome, &num, NULL);
  }
  /* BUTTON BACK */
  if(num == BUTTON_BACK)
  {
    xQueueSendFromISR(interputQueueButtonBack, &num, NULL);
  }
  /* BUTTON NEXT */
  if(num == BUTTON_NEXT)
  {
    xQueueSendFromISR(interputQueueButtonNext, &num, NULL); 
  }
}

void vTimerCallback( TimerHandle_t xTimer )
 {
   uint32_t ID;
   ID = (uint32_t )pvTimerGetTimerID(xTimer);
   BaseType_t xHigherPriorityTaskWoken = pdFALSE;
   if(ID == 0)  
   {
     status_button[0] = 1;
     xEventGroupSetBitsFromISR(xButtonEventGroup, BUTTON_BACK_LONG_PRESS, &xHigherPriorityTaskWoken);
   }
   else if (ID == 1)
   {
     status_button[1] = 1;
      xEventGroupSetBitsFromISR(xButtonEventGroup, BUTTON_HOME_LONG_PRESS, &xHigherPriorityTaskWoken);
   }
 }

void buttonBackTask(void *params)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int num;
    while (true)
    {
        if (xQueueReceive(interputQueueButtonBack, &num, portMAX_DELAY))
        {
            status_button[0] = 0;
            // disable the interrupt
            gpio_isr_handler_remove(num);

            // wait some time while we check for the button to be released
            xTimerStart(xTimers[0], 0);
            do 
            {
                vTaskDelay(20 / portTICK_PERIOD_MS);
            } while(gpio_get_level(num) == 1);
            xTimerStop(xTimers[0], 0);
            //do some work    
            if(!status_button[0])
            {
              //press
              xEventGroupSetBitsFromISR(xButtonEventGroup, BUTTON_BACK_PRESS, &xHigherPriorityTaskWoken); 
              //ESP_LOGI(TAG, "HOME PRESS");
            }         
            // re-enable the interrupt 
             gpio_isr_handler_add(num, gpio_input_handler, (void *)num);
        }
    }
}
void buttonHomeTask(void *params)
{
    int num;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    while (true)
    {
        if (xQueueReceive(interputQueueButtonHome, &num, portMAX_DELAY))
        {
            status_button[1] = 0;
            // disable the interrupt
            gpio_isr_handler_remove(num);

            // wait some time while we check for the button to be released
            xTimerStart(xTimers[1], 0);
            do 
            {
                vTaskDelay(20 / portTICK_PERIOD_MS);
            } while(gpio_get_level(num) == 1);
            xTimerStop(xTimers[1], 0);
            //do some work
            if(!status_button[1])
            {
              //press
              xEventGroupSetBitsFromISR(xButtonEventGroup, BUTTON_HOME_PRESS, &xHigherPriorityTaskWoken);  
              static int t = 1;
              gpio_set_level(4, t);
              t = 1 - t;         
            }

            // re-enable the interrupt 
             gpio_isr_handler_add(num, gpio_input_handler, (void *)num);

        }
    }
}
void buttonNextTask(void *params)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int num;
    while (true)
    {
        if (xQueueReceive(interputQueueButtonNext, &num, portMAX_DELAY))
        {
            // disable the interrupt
            gpio_isr_handler_remove(num);

            // wait some time while we check for the button to be released
            do 
            {
                vTaskDelay(20 / portTICK_PERIOD_MS);
            } while(gpio_get_level(num) == 1);

            //do some work
            xEventGroupSetBitsFromISR(xButtonEventGroup, BUTTON_NEXT_PRESS, &xHigherPriorityTaskWoken);

            // re-enable the interrupt 
             gpio_isr_handler_add(num, gpio_input_handler, (void *)num);

        }
    }
}

void initGPIO(void)
{
  //input
  input_io_creat(12, ANY_EDGE);
  input_io_creat(15, ANY_EDGE);
  input_io_creat(13, ANY_EDGE);
  input_set_callback(input_event_callback);
  
  //output
  output_io_create(2);
  output_io_create(4);
  output_io_create(5);
  /**
   *@brief Create timer soft for button press 
   * ID = 0 button BACK
   * ID = 1 button HOME
   * ID = 2 
   */

  xTimers[ 0 ] = xTimerCreate("Timer", 1000 / portTICK_PERIOD_MS, pdFALSE, ( void * ) 0,vTimerCallback);
  xTimers[ 1 ] = xTimerCreate("Timer", 1000 / portTICK_PERIOD_MS, pdFALSE, ( void * ) 1,vTimerCallback);

}

void home()
{
  ssd1306_clear(0);
  ssd1306_select_font(0, 1);
	ssd1306_draw_string(0, 20, 30, "HOME", 2, 0);
	ssd1306_refresh(0, true);
  xEventGroupSetBits(xDisplayEventGroup, DISPLAY_HOME);
  EventBits_t  evenBit = xEventGroupGetBits(  xDisplayEventGroup );
  ESP_LOGI(TAG, "%d",(int)evenBit);
}

void task_display_home()
{
    while(1)
    {
      EventBits_t uxBits = xEventGroupWaitBits(xButtonEventGroup, BUTTON_BACK_LONG_PRESS | BUTTON_HOME_LONG_PRESS, pdTRUE, pdFALSE, portMAX_DELAY);
      if((uxBits & BUTTON_HOME_LONG_PRESS) != 0)
      {
        ssd1306_clear(0);
        ssd1306_select_font(0, 1);
		    ssd1306_draw_string(0, 20, 30, "home", 2, 0);
	    	ssd1306_refresh(0, true);
        xEventGroupSetBits(xDisplayEventGroup, DISPLAY_HOME); 
      }
      else
      {
        xEventGroupWaitBits(xDisplayEventGroup, DISPLAY_MENU_1 | DISPLAY_MENU_2 , pdTRUE, pdFALSE, portMAX_DELAY);
        ssd1306_clear(0);
        ssd1306_select_font(0, 1);
		    ssd1306_draw_string(0, 20, 30, "home1", 2, 0);
	      ssd1306_refresh(0, true);
        xEventGroupSetBits(xDisplayEventGroup, DISPLAY_HOME);
      }
    }
}

void task_display_menu_1(void *param)
{
  while(1)
  {
    EventBits_t uxBits = xEventGroupWaitBits(xButtonEventGroup, BUTTON_BACK_PRESS | BUTTON_BACK_LONG_PRESS | BUTTON_NEXT_PRESS | BUTTON_HOME_PRESS, pdTRUE, pdFALSE, portMAX_DELAY);
    if((uxBits & BUTTON_BACK_LONG_PRESS) != 0)
    {
      xEventGroupWaitBits(xDisplayEventGroup, DISPLAY_GARDEN_1, pdTRUE, pdFALSE, portMAX_DELAY);
      ssd1306_clear(0);
      ssd1306_select_font(0, 1);
		  ssd1306_draw_string(0, 10, 30, "Garden 1" , 2, 0);
		  ssd1306_refresh(0, true);
      xEventGroupSetBits(xDisplayEventGroup, DISPLAY_MENU_1);
    }
    if((uxBits & (BUTTON_BACK_PRESS | BUTTON_NEXT_PRESS))  != 0)
    {
      xEventGroupWaitBits(xDisplayEventGroup, DISPLAY_MENU_2, pdTRUE, pdFALSE, portMAX_DELAY);
      ssd1306_clear(0);
      ssd1306_select_font(0, 1);
		  ssd1306_draw_string(0, 10, 30, "Garden 1" , 2, 0);
		  ssd1306_refresh(0, true);
      xEventGroupSetBits(xDisplayEventGroup, DISPLAY_MENU_1);
    }
    if((uxBits & BUTTON_HOME_PRESS) != 0)
    {
      xEventGroupWaitBits(xDisplayEventGroup, DISPLAY_HOME, pdTRUE, pdFALSE, portMAX_DELAY);
      ssd1306_clear(0);
      ssd1306_select_font(0, 1);
		  ssd1306_draw_string(0, 10, 30, "Garden 1" , 2, 0);
		  ssd1306_refresh(0, true);
      xEventGroupSetBits(xDisplayEventGroup, DISPLAY_MENU_1);
    }
  }
}

void task_display_menu_2(void *param)
{
  while(1)
  {
    EventBits_t uxBits = xEventGroupWaitBits(xButtonEventGroup, BUTTON_BACK_PRESS | BUTTON_BACK_LONG_PRESS | BUTTON_NEXT_PRESS , pdTRUE, pdFALSE, portMAX_DELAY);
    if((uxBits & BUTTON_BACK_LONG_PRESS) != 0)
    {
      xEventGroupWaitBits(xDisplayEventGroup, DISPLAY_GARDEN_2, pdTRUE, pdFALSE, portMAX_DELAY);
      ssd1306_clear(0);
      ssd1306_select_font(0, 1);
		  ssd1306_draw_string(0, 20, 30, "Garden 2" , 2, 0);
		  ssd1306_refresh(0, true);
      xEventGroupSetBits(xDisplayEventGroup, DISPLAY_MENU_2);
    }
    if((uxBits & (BUTTON_BACK_PRESS | BUTTON_NEXT_PRESS))  != 0)
    {
      xEventGroupWaitBits(xDisplayEventGroup, DISPLAY_MENU_1, pdTRUE, pdFALSE, portMAX_DELAY);
      ssd1306_clear(0);
      ssd1306_select_font(0, 1);
		  ssd1306_draw_string(0, 20, 30, "Garden 2" , 2, 0);
		  ssd1306_refresh(0, true);
      xEventGroupSetBits(xDisplayEventGroup, DISPLAY_MENU_2);
    }
  }
}

void task_display_garden_1(void *param)
{
  while(1)
  {
    xEventGroupWaitBits(xButtonEventGroup, BUTTON_HOME_PRESS, pdTRUE, pdFALSE, portMAX_DELAY);
    xEventGroupWaitBits(xDisplayEventGroup, DISPLAY_MENU_1, pdTRUE, pdFALSE, portMAX_DELAY);
    ssd1306_clear(0);
    ssd1306_select_font(0, 1);
		ssd1306_draw_string(0, 20, 30, "Info garden 1" , 2, 0);
		ssd1306_refresh(0, true);
    xEventGroupSetBits(xDisplayEventGroup, DISPLAY_GARDEN_1);
  }
}

void task_display_garden_2(void *param)
{
  while(1)
  {
    xEventGroupWaitBits(xButtonEventGroup, BUTTON_HOME_PRESS, pdTRUE, pdFALSE, portMAX_DELAY);
    xEventGroupWaitBits(xDisplayEventGroup, DISPLAY_MENU_2, pdTRUE, pdFALSE, portMAX_DELAY);
    ssd1306_clear(0);
    ssd1306_select_font(0, 1);
		ssd1306_draw_string(0, 20, 30, "Info garden 2" , 2, 0);
		ssd1306_refresh(0, true);
    xEventGroupSetBits(xDisplayEventGroup, DISPLAY_GARDEN_2);
  }
}

void task_display_home_garden_machine(void *param)
{
  
}

void app_main()
{

  esp_log_level_set(TAG, ESP_LOG_DEBUG);
  xButtonEventGroup = xEventGroupCreate();
  xDisplayEventGroup = xEventGroupCreate();

  interputQueueButtonBack = xQueueCreate(10, sizeof(int));
  interputQueueButtonHome = xQueueCreate(10, sizeof(int));
  interputQueueButtonNext = xQueueCreate(10, sizeof(int));

  initGPIO();

  xTaskCreate(&buttonBackTask, "buttonNextBack", 1024, NULL, 1, NULL);
  xTaskCreate(&buttonHomeTask, "buttonNextHome", 1024, NULL, 1, NULL);
  xTaskCreate(&buttonNextTask, "buttonNextNext", 1024, NULL, 1, NULL);
  //connectionSemaphore = xSemaphoreCreateBinary();
  ssd1306_init(0, 19, 22);


  home();
  xTaskCreate(&task_display_home, "display home", 1024, NULL, 4, NULL);
  xTaskCreate(&task_display_menu_1, "task_display_menu_1", 1024, NULL, 4, NULL);
  xTaskCreate(&task_display_menu_2, "task_display_menu_2", 1024, NULL, 4, NULL);
  xTaskCreate(&task_display_garden_1, "task_display_garden_1", 1024, NULL, 4, NULL);
  xTaskCreate(&task_display_garden_2, "task_display_garden_2", 1024, NULL, 4, NULL);

  //wifiInit();
  //xTaskCreate(&OnConnected, "handel comms", 1024 * 3, NULL, 5, NULL);
  
  //vTaskStartScheduler();
  
}