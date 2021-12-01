#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
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




#define NUM_TIMERS 3


xSemaphoreHandle connectionSemaphore;



xQueueHandle interputQueueButtonBack;
xQueueHandle interputQueueButtonHome;
xQueueHandle interputQueueButtonNext;

TimerHandle_t xTimers[ NUM_TIMERS ];



typedef struct infoGarden
{
  uint8_t ID;
  uint8_t data[6];
  uint8_t control;
} info_garden_t;

static uint8_t status_button[3];

static info_garden_t info_garden[2];
static uint8_t display_machine_number;


void IRAM_ATTR gpio_input_handler(void *args);

void task_display_home(void);
void task_display_menu(info_garden_t *info_garden);
void task_display_garden(info_garden_t *info_garden);
void task_display_garden_machine(info_garden_t *info_garden, uint8_t machine_number);



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
     status_button[0] = 2;

   }
   else if (ID == 1)
   {
     status_button[1] = 2;

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
              status_button[0] = 1;
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
              status_button[1] = 1;        
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
            status_button[2] = 1;

            // re-enable the interrupt 
             gpio_isr_handler_add(num, gpio_input_handler, (void *)num);

        }
    }
}

void initGPIO(void)
{
  //input
  input_io_creat(12, LO_TO_HI);
  input_io_creat(15, LO_TO_HI);
  input_io_creat(13, LO_TO_HI);
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
  
  for(int i = 0; i<2; i++)
  {
    info_garden[i].ID = i + 1;
    //info_garden->data = 0;
    //info_garden->control = 0;
  }
  display_machine_number = 0;


}
void resetButton(void)
{
  for(int i = 0; i<3; i++)
  {
    status_button[i] = 0;
  }
}

void task_display_home()
{
  
  ssd1306_clear(0);
  ssd1306_select_font(0, 1);
	ssd1306_draw_string(0, 20, 30, "HOME", 2, 0);
	ssd1306_refresh(0, true);
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  resetButton();
  while(1)
  {
    
    if(status_button[1] == 1)
    {
      status_button[1] = 0;
      task_display_menu(&info_garden[0]);
    }
  }
}


void task_display_menu(info_garden_t *_info_garden)
{
  char string_temp[64];
  sprintf(string_temp, "garden %d",_info_garden->ID);
  ssd1306_clear(0);
  ssd1306_select_font(0, 1);
  ssd1306_draw_string(0, 10, 30, string_temp , 2, 0);
  ssd1306_refresh(0, true);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  resetButton();
  uint8_t temp = (_info_garden->ID  == 1)?(1):(0);
  while(1)
  {
    if(status_button[1] == 1)
    {
      
      status_button[1] = 0;
      task_display_garden(&info_garden[_info_garden->ID - 1]);
    }
    else if((status_button[1] == 2) || (status_button[0] == 2))
    {
      status_button[0] = 0;
      status_button[1] = 0;
      task_display_home();
    }
    else if(status_button[0] + status_button[2] != 0)
    {
      status_button[0] = 0;
      status_button[2] = 0;
      task_display_menu(&info_garden[temp]);
    }
  }
}





void task_display_garden(info_garden_t *_info_garden)
{
    char string_temp[128];
    sprintf(string_temp,"info garden %d",_info_garden->ID);
    ssd1306_clear(0);
    ssd1306_select_font(0, 1);
		ssd1306_draw_string(0, 20, 30, string_temp , 2, 0);
		ssd1306_refresh(0, true);
    resetButton();
    while(1)
    {
      if(status_button[1] == 2)
      {
        status_button[1] = 0;
        task_display_home();
      }
      else if(status_button[0] == 2)
      {
        status_button[0] = 0;
        task_display_menu(&info_garden[_info_garden->ID - 1]);
      }
      else if(status_button[0] == 1)
      {
        status_button[0] = 0;
        task_display_garden_machine(&info_garden[_info_garden->ID - 1], 0);
      }
    }
}

void task_display_garden_machine(info_garden_t *_info_garden, uint8_t _machine_number)
{
    char string_temp[128];
    uint8_t machine_number_temp = display_machine_number;
    sprintf(string_temp,"info garden machine %d",_info_garden->ID);
    ssd1306_clear(0);
    ssd1306_select_font(0, 1);
		ssd1306_draw_string(0, 20, 30, string_temp , 2, 0);
    ssd1306_draw_rectangle(0, 103, 2 + 25*display_machine_number, 25, 25, 1);
		ssd1306_refresh(0, true);
    resetButton();
    while(1)
    {
      if(status_button[1] == 2)
      {
        status_button[1] = 0;
        task_display_home();
      }
      else if(status_button[0] == 2)
      {
        status_button[0] = 0;
        task_display_menu(&info_garden[_info_garden->ID - 1]);
      }
      else if(status_button[0] == 1)
      {
        if(machine_number_temp-- == 1)
        {
          machine_number_temp = 1;
        }
        task_display_garden_machine(&info_garden[_info_garden->ID - 1], machine_number_temp);
      }
      else if(status_button[2] == 1)
      {
        if(machine_number_temp++ == 4)
        {
          machine_number_temp = 1;
        }
        task_display_garden_machine(&info_garden[_info_garden->ID - 1], machine_number_temp);
      }
    }
}








void app_main()
{

  esp_log_level_set(TAG, ESP_LOG_DEBUG);




  interputQueueButtonBack = xQueueCreate(10, sizeof(int));
  interputQueueButtonHome = xQueueCreate(10, sizeof(int));
  interputQueueButtonNext = xQueueCreate(10, sizeof(int));



  initGPIO();

  xTaskCreate(&buttonBackTask, "buttonNextBack", 1024, NULL, 1, NULL);
  xTaskCreate(&buttonHomeTask, "buttonNextHome", 1024, NULL, 1, NULL);
  xTaskCreate(&buttonNextTask, "buttonNextNext", 1024, NULL, 1, NULL);
  //connectionSemaphore = xSemaphoreCreateBinary();
  ssd1306_init(0, 19, 22);


  task_display_home();


  //wifiInit();
  //xTaskCreate(&OnConnected, "handel comms", 1024 * 3, NULL, 5, NULL);
  
  //vTaskStartScheduler();
  
}