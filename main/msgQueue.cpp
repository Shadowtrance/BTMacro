#include "msgQueue.h"
#include "hidController.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

extern HidController hidController;

QueueHandle_t msgQueue;

void msgTask(void *pvParam)
{
  int itemNum;
  while (1)
  {
    if (xQueueReceive(msgQueue, &itemNum, portMAX_DELAY) == pdTRUE)
    {
      hidController.triggerTask(itemNum);
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void beginMsgTask()
{
  msgQueue = xQueueCreate(10, sizeof(int));
  xTaskCreate(msgTask, "msgTask", 1024, nullptr, 1, nullptr);
}