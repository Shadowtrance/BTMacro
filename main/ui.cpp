#include "ui.h"
#include "helpers.h"
#include "sdCard.h"
#include "msgQueue.h"
#include "esp_log.h"

const char *btnmMapDefault[] = {
    "1", "2", "3", "\n",
    "4", "5", "6", ""};

static const char *TAG = "[UI]";

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
    ESP_LOGI(TAG, "Button pressed: %lu", id);
    xQueueSend(msgQueue, &id, portMAX_DELAY);
  }
}

void makeGrid()
{
  ESP_LOGI(TAG, "Creating UI grid");
  // Acquire LVGL lock before creating UI elements
  lv_lock();
  char button_map_path[64];
  snprintf(button_map_path, sizeof(button_map_path), "%s/button_map.json", MOUNT_POINT);
  const char **dynamic_map = create_button_map_from_json(button_map_path);
  if (dynamic_map == NULL)
  {
    dynamic_map = btnmMapDefault;
  }
  static lv_style_t style_buttons;
  lv_style_init(&style_buttons);
  lv_style_set_text_font(&style_buttons, &lv_font_montserrat_48);

  lv_obj_t *btnm = lv_btnmatrix_create(lv_screen_active());
  lv_btnmatrix_set_map(btnm, dynamic_map);
  lv_obj_add_style(btnm, &style_buttons, LV_PART_ITEMS);
  lv_obj_set_size(btnm, 800, 480);
  lv_obj_align(btnm, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_opa(btnm, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(btnm, 0, LV_PART_MAIN);
  lv_obj_add_event_cb(btnm, keyEventCb, LV_EVENT_VALUE_CHANGED, NULL);
  lv_unlock();
}
