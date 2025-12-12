#include "hidController.h"
#include "displayConfig.h"
#include "helpers.h"
#include "sdCard.h"
#include "ui.h"
#include "msgQueue.h"
#include "myShortcut.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "nvs_flash.h"

#include "lvgl.h"

HidController hidController;
bool isConnected;
TaskHandle_t bleTask;

void updateConnection(bool isOn)
{
  if (isConnected != isOn)
  {
        isConnected = isOn;
    }
}

void beginHidController()
{
    hidController.bindCallback(updateConnection);
    hidController.begin();
}

void loopTask(void *pvParameters)
{
  for (;;)
{
    hidController.loop();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void taskStartup(void)
{
  if (bleTask)
  {
      return;
    }
  
    xTaskCreate(loopTask, "loopTask", 2048, NULL, configMAX_PRIORITIES - 3, &bleTask);
}

extern "C" void app_main(void)
{
    isConnected = false;
    
    // Initialize NVS first - required for Bluetooth
    esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    suntonEsp32s3LcdInit();

  initialize_sd_card();
  // verify_filesystem();

    // Give LVGL task time to start before creating UI elements
    vTaskDelay(pdMS_TO_TICKS(100));
    
    makeGrid();

  MyShortcut::loadShortcuts();

    beginMsgTask();
    beginHidController();
    
    taskStartup();  // Start the loop task
}
