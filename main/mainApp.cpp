#include "hidController.h"
#include "displayConfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lvgl.h"

static const char *TAG = "esp_hid_device_main";

static HidController hidController;
QueueHandle_t msgQueue;
bool isConnected;
TaskHandle_t bleTask;

lv_obj_t *btnm;

const char *btnmMap[] = {
    LV_SYMBOL_PREV,
    LV_SYMBOL_PLAY,
    LV_SYMBOL_NEXT,
    LV_SYMBOL_MUTE, "\n",
    LV_SYMBOL_PLUS,
    LV_SYMBOL_MINUS,
    "1",
    "2", "\n",
    "3",
    "4",
    "5",
    "6", ""
};

void updateConnection(bool isOn)
{
    if (isConnected != isOn) {
        isConnected = isOn;
    }
}

void beginHidController() {
    hidController.bindCallback(updateConnection);
    hidController.begin();
}

void msgTask(void *pvParam) {
    int itemNum;
    while (1) {
        if (xQueueReceive(msgQueue, &itemNum, portMAX_DELAY) == pdTRUE) {
            hidController.triggerTask(itemNum);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void beginMsgTask() {
    msgQueue = xQueueCreate(10, sizeof(int));
    xTaskCreate(msgTask, "msgTask", 1024, nullptr, 1, nullptr);
}

void loopTask(void *pvParameters)
{
  for (;;) {
    hidController.loop();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void taskStartup(void) {
    if (bleTask) {
      return;
    }
  
    xTaskCreate(loopTask, "loopTask", 2048, NULL, configMAX_PRIORITIES - 3, &bleTask);
}

void keyEventCb(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *obj = lv_event_get_target_obj(e);
  if (code == LV_EVENT_VALUE_CHANGED)
  {
    uint32_t id = lv_btnmatrix_get_selected_btn(obj);
    const char *txt = lv_btnmatrix_get_btn_text(obj, id);
    LV_UNUSED(txt);
    LV_LOG_USER("%s was pressed\n", txt);
    ESP_LOGI(TAG, "button pressed: %lu", id);
    xQueueSend(msgQueue, &id, portMAX_DELAY);
  }
}

void makeGrid()
{
  // Acquire LVGL lock before creating UI elements
  lv_lock();
  btnm = lv_btnmatrix_create(lv_scr_act());
  lv_btnmatrix_set_map(btnm, btnmMap);
  lv_obj_set_size(btnm, 800, 480);
  lv_obj_align(btnm, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_opa(btnm, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(btnm, keyEventCb, LV_EVENT_CLICKED, NULL);
  lv_unlock();
}

extern "C" void app_main(void)
{
    isConnected = false;
    
    // Initialize NVS first - required for Bluetooth
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    suntonEsp32s3LcdInit();

    // Give LVGL task time to start before creating UI elements
    vTaskDelay(pdMS_TO_TICKS(100));
    
    makeGrid();

    beginMsgTask();
    beginHidController();
    
    taskStartup();  // Start the loop task
}
