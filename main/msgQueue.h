#ifndef MSG_QUEUE_H
#define MSG_QUEUE_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

extern QueueHandle_t msgQueue;

void msgTask(void *pvParam);
void beginMsgTask();

#endif