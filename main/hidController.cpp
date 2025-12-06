#include "hidController.h"
#include "myShortcut.h"
#include "hidKeyboard.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define ALIVE_INTERVAL 100

HidController::HidController() {
    isConnected = false;
    previousTimestamp = 0;
}

void HidController::begin(){
    hid_keyboard_init();
}

unsigned long IRAM_ATTR millis() {
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

void HidController::loop(){
    unsigned long currentTimestamp = millis();
    if (currentTimestamp - previousTimestamp > ALIVE_INTERVAL) {
      previousTimestamp = currentTimestamp;
      isConnected = hid_keyboard_is_connected();
      callback(isConnected); 
    }
}

void sendTask(void* pvParam){
    MyShortcut* _myShortcut = reinterpret_cast<MyShortcut*>(pvParam);
    _myShortcut->Action();
    vTaskDelay(pdMS_TO_TICKS(100));
    _myShortcut->ReleaseAllKeys();
    
    delete _myShortcut;
    
    vTaskDelete(NULL);
}

void HidController::triggerTask(int caseNum){
    if(!isConnected) return;
  
    MyShortcut* myShortcut = new MyShortcut(caseNum);
    
    xTaskCreate(
      sendTask, 
      "sendTask",
      4096,     
      (void*) myShortcut,
      1,
      NULL);
}
