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
#include "connect.h"
#include "ssd1306.h"
#include "fonts.h"
#include "gpio.h"
#include "mqtt_client.h"
#include "mirf.h"



#define TAG "DATA"


#define BUTTON_BACK 4
#define BUTTON_HOME 15
#define BUTTON_NEXT 27




#define NUM_TIMERS 3

//For connect
//xSemaphoreHandle connectionSemaphore;
xQueueHandle readingQueue;
TaskHandle_t taskHandle;

const uint32_t WIFI_CONNECTED = BIT1;
const uint32_t MQTT_CONNECTED = BIT2;
const uint32_t MQTT_PUBLISHED = BIT3;

static char temp_control_receive[2];
static bool check_staus_control_receive = 0;

//For hardware
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

//Data lora nfr24l01
NRF24_t dev;



void IRAM_ATTR gpio_input_handler(void *args);

void task_display_home(void);
void task_display_menu(info_garden_t *info_garden);
void task_display_garden(info_garden_t *_info_garden);
void task_display_garden_machine(info_garden_t *info_garden, uint8_t machine_number);

/*

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

*/
void mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
  switch (event->event_id)
  {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    xTaskNotify(taskHandle, MQTT_CONNECTED, eSetValueWithOverwrite);
    break;
  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
    break;
  case MQTT_EVENT_SUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_UNSUBSCRIBED:
    ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
    break;
  case MQTT_EVENT_PUBLISHED:
    ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
    xTaskNotify(taskHandle, MQTT_PUBLISHED, eSetValueWithOverwrite);
    break;  
  case MQTT_EVENT_DATA:
    ESP_LOGI(TAG, "MQTT_EVENT_DATA");
    printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
    //printf("DATA=%.*s\r\n", event->data_len, event->data);
    sprintf(temp_control_receive, "%.*s", event->data_len, event->data);
    check_staus_control_receive = 1;
    break;
  case MQTT_EVENT_ERROR:
    ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
    break;
  default:
    ESP_LOGI(TAG, "Other event id:%d", event->event_id);
    break;
  }
}

void string2hexString(char* input, char* output)
{
    int loop;
    int i; 
    
    i=0;
    loop=0;
    
    while(input[loop] != '\0')
    {
        sprintf((char*)(output+i),"%02X", input[loop]);
        loop+=1;
        i+=2;
    }
    //insert NULL at the end of the output string
    output[i++] = '\0';
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
  mqtt_event_handler_cb(event_data);
}

void MQTTLogic(int sensorReading)
{
  uint32_t command = 0;
  esp_mqtt_client_config_t mqttConfig  = {
    .uri = "mqtt://test.mosquitto.org:1883"
  };
  esp_mqtt_client_handle_t client = NULL;

  while(1)
  {
    xTaskNotifyWait(0,0,&command, portMAX_DELAY);
    switch (command)
    {
    case WIFI_CONNECTED:
      client = esp_mqtt_client_init(&mqttConfig);
      esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
      esp_mqtt_client_start(client);
      break;
    case MQTT_CONNECTED:
      vTaskDelay(100 / portTICK_PERIOD_MS);
      esp_mqtt_client_subscribe(client, "thanh9112/dht11", 2);
      char data[50]; 
      sprintf(data,"%d",sensorReading);
      printf("sending data %d", sensorReading);
      // vTaskDelay(5000 / portTICK_PERIOD_MS);
      esp_mqtt_client_publish(client, "thanh9112/dht12", data, strlen(data), 2, false);
     
      break;
    case MQTT_PUBLISHED:
      esp_mqtt_client_stop(client);
      esp_mqtt_client_destroy(client);
      esp_wifi_stop();
      return;
    default:
      break;
    }
  }
}


void OnConnected(void *para)
{
  
  while (true)
  {
    int senserReading;
    if(xQueueReceive(readingQueue, &senserReading, portMAX_DELAY))
    {
      ESP_ERROR_CHECK(esp_wifi_start());
      MQTTLogic(senserReading);
    }
  }
}

void genrerateReading()
{
  while(1)
  {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    
    xQueueSend(readingQueue, &random, 2000 / portTICK_PERIOD_MS);
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
    int x = 1;
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
            
            gpio_set_level(2, x);
            x = 1 - x;
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
  input_io_creat(BUTTON_BACK, LO_TO_HI);
  input_io_creat(BUTTON_HOME, LO_TO_HI);
  input_io_creat(BUTTON_NEXT, LO_TO_HI);
  input_set_callback(input_event_callback);
  
  //output
  output_io_create(2);
  //output_io_create(4);
  //output_io_create(5);
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
    info_garden[i].control = i + 0x15;
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
	ssd1306_draw_string(0, 20, 30, "HOME HOME", 2, 0);
	ssd1306_refresh(0, true);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
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
  vTaskDelay(500 / portTICK_PERIOD_MS);
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
    for(int i = 0; i<5; i++)
    {
      if(((1<<i)&(_info_garden->control)) == (1<<i))
      {
        ssd1306_draw_string(0, 2 + 25*i, 43, "ON" , 2, 0);
      }
      else
      {
        ssd1306_draw_string(0, 2 + 25*i, 43, "OFF" , 2, 0);
      }
    }


		ssd1306_refresh(0, true);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    resetButton();
    while(1)
    {
      if(status_button[1] == 1)
      {
        status_button[1] = 0;
        task_display_garden_machine(&info_garden[_info_garden->ID - 1], 0);
      }
      else if(status_button[1] == 2)
      {
        status_button[1] = 0;
        task_display_home();
      }
      else if(status_button[0] == 2)
      {
        status_button[0] = 0;
        task_display_menu(&info_garden[_info_garden->ID - 1]);
      }

    }
}

void task_display_garden_machine(info_garden_t *_info_garden, uint8_t _machine_number)
{
    char string_temp[128];
    char temp_convert[2];
    if(check_staus_control_receive)
    {
      check_staus_control_receive = 0;
      string2hexString(temp_control_receive, temp_convert);
        for(int i = 0; i < 2; i++)
        {
          _info_garden[i].control = temp_convert[i];
        }
    }
    
    int8_t machine_number_temp = _machine_number;
    sprintf(string_temp,"info garden machine %d",_info_garden->ID);
    ssd1306_clear(0);
    ssd1306_select_font(0, 1);
		ssd1306_draw_string(0, 20, 20, string_temp , 2, 0);
    for(int i = 0; i<5; i++)
    {
      if(((1<<i)&(_info_garden->control)) == (1<<i))
      {
        ssd1306_draw_string(0, 2 + 25*i, 43, "ON" , 2, 0);
      }
      else
      {
        ssd1306_draw_string(0, 2 + 25*i, 43, "OFF" , 2, 0);
      }
    }
    ssd1306_draw_rectangle(0, 0 + 25*machine_number_temp, 39, 25, 25, 1);

		ssd1306_refresh(0, true);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    resetButton();
    while(1)
    {
      
      if(status_button[1] == 1)
      {
        status_button[1] = 0;
        _info_garden->control = _info_garden->control ^ (1 << machine_number_temp);
        task_display_garden_machine(&info_garden[_info_garden->ID - 1], machine_number_temp);
      }
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
        machine_number_temp = machine_number_temp - 1;
        if(machine_number_temp < 0)
        {
          machine_number_temp = 4;
        }
        task_display_garden_machine(&info_garden[_info_garden->ID - 1], machine_number_temp);
      }
      else if(status_button[2] == 1)
      {
        status_button[2] = 0;
        machine_number_temp = machine_number_temp + 1;;
        if(machine_number_temp > 4)
        {
          machine_number_temp = 0;
        } 
        task_display_garden_machine(&info_garden[_info_garden->ID - 1], machine_number_temp);
      }
      else if(status_button[1] == 1)
      {
        status_button[1] = 0;
        _info_garden->control = _info_garden->control ^ (1 << machine_number_temp);
        task_display_garden_machine(&info_garden[_info_garden->ID - 1], machine_number_temp);
      }
    }
}

//NRF24L01 receive
void receiver()
{
  uint8_t rbuff[7];
	uint8_t payload = 7;
	uint8_t channel = 90;
	Nrf24_config(&dev, channel, payload);
	ESP_LOGI(pcTaskGetTaskName(0), "Listening...");
  
	while(1) {
		if (Nrf24_dataReady(&dev)) 
    { 
			Nrf24_getData(&dev, rbuff);
			ESP_LOGI(pcTaskGetTaskName(0), "Got data :%s",rbuff);
      if(rbuff[6] == 0)
      {
        //do something
      }
      else
      {
        //do something
      }
		}
		vTaskDelay(5);
	}
}

//NRF24L01 transmiter
void transmitter_gardent(void *pvParameters)
{
	uint8_t payload = 2;
	uint8_t channel = 90;
	Nrf24_config(&dev, channel, payload);
	while(1) {
		uint8_t tbuff[2];
    tbuff[0] = info_garden[0].control;
    tbuff[1] = info_garden[1].control;
		Nrf24_send(&dev, tbuff);				   //Send instructions, send random number value
		vTaskDelay(5);
		ESP_LOGI(pcTaskGetTaskName(0), "Wait for sending.....");
		if (Nrf24_isSend(&dev)) {
			ESP_LOGI(pcTaskGetTaskName(0),"Send success");
		} else {
			ESP_LOGI(pcTaskGetTaskName(0),"Send fail:");
		}
    receiver();
		vTaskDelay(60000/portTICK_PERIOD_MS);
	}
}



void Nrf24L01_config(void)
{
	ESP_LOGI(pcTaskGetTaskName(0), "Start");
	ESP_LOGI(pcTaskGetTaskName(0), "CONFIG_CE_GPIO=%d",CONFIG_CE_GPIO);
	ESP_LOGI(pcTaskGetTaskName(0), "CONFIG_CSN_GPIO=%d",CONFIG_CSN_GPIO);
	spi_master_init(&dev, CONFIG_CE_GPIO, CONFIG_CSN_GPIO, CONFIG_MISO_GPIO, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO);

  Nrf24_setTADDR(&dev, (uint8_t *)"ABCDE");
  Nrf24_setRADDR_P1(&dev, (uint8_t *)"BCDEF");
  //Nrf24_setRADDR_P2(&dev, (uint8_t *)"CDEFG");
  Nrf24_printDetails(&dev);
}



void app_main()
{

  esp_log_level_set(TAG, ESP_LOG_DEBUG);

  interputQueueButtonBack = xQueueCreate(10, sizeof(int));
  interputQueueButtonHome = xQueueCreate(10, sizeof(int));
  interputQueueButtonNext = xQueueCreate(10, sizeof(int));


  readingQueue = xQueueCreate(sizeof(int), 10);

  initGPIO();
  wifiInit();
  ssd1306_init(0, 21, 22);
  Nrf24L01_config();
  xTaskCreate(OnConnected, "handel comms", 1024 * 4, NULL, 5, &taskHandle);
  xTaskCreate(task_display_home,"display on sdd1306", 1024 * 15, NULL, 1, NULL);
  //xTaskCreate(genrerateReading, "reading input periodic", 1024 * 2, NULL, 6, NULL);


  xTaskCreate(buttonBackTask, "buttonNextBack", 512, NULL, 10, NULL);
  xTaskCreate(buttonHomeTask, "buttonNextHome", 512, NULL, 10, NULL);
  xTaskCreate(buttonNextTask, "buttonNextNext", 512, NULL, 10, NULL);


  xTaskCreate(transmitter_gardent, "TRANS garden", 1024*3, NULL, 8, NULL);;
  

  //vTaskStartScheduler();
  
}