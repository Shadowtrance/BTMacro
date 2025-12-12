#include "hidKeyboard.h"
#include "espHiddPrfApi.h"
#include "hidDev.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_ble_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "[HID KEYBOARD]";
static bool s_connected = false;

static esp_ble_adv_data_t hidd_adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0020,
    .max_interval = 0x0040,
    .appearance = 0x03C1,  // Keyboard
    .manufacturer_len = 0,
    .p_manufacturer_data = NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = 0,
    .p_service_uuid = NULL,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t hidd_adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

static void hidd_event_callback(esp_hidd_cb_event_t event, esp_hidd_cb_param_t *param)
{
    switch(event) {
        case ESP_HIDD_EVENT_REG_FINISH: {
            if (param->init_finish.state == ESP_HIDD_INIT_OK) {
                ESP_LOGI(TAG, "HID device profile initialized");
                esp_ble_gap_set_device_name("Macro Deck");
                esp_ble_gap_config_adv_data(&hidd_adv_data);
            }
            break;
        }
        case ESP_BAT_EVENT_REG: {
            break;
        }
        case ESP_HIDD_EVENT_DEINIT_FINISH:
            break;
        case ESP_HIDD_EVENT_BLE_CONNECT: {
            ESP_LOGI(TAG, "Device connected");
            s_connected = true;
            break;
        }
        case ESP_HIDD_EVENT_BLE_DISCONNECT: {
            ESP_LOGI(TAG, "Device disconnected");
            s_connected = false;
            ESP_LOGI(TAG, "Waiting 1 second before re-advertising");
            vTaskDelay(pdMS_TO_TICKS(1000));
            ESP_LOGI(TAG, "Restarting advertising");
            esp_ble_gap_start_advertising(&hidd_adv_params);
            break;
        }
        case ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT: {
            ESP_LOGD(TAG, "%s, ESP_HIDD_EVENT_BLE_VENDOR_REPORT_WRITE_EVT", __func__);
            break;
        }
        case ESP_HIDD_EVENT_BLE_LED_REPORT_WRITE_EVT: {
            ESP_LOGD(TAG, "LED state: %d", param->led_write.data[0]);
            break;
        }
        default:
            break;
    }
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&hidd_adv_params);
        ESP_LOGI(TAG, "Advertising started");
        break;
    case ESP_GAP_BLE_SEC_REQ_EVT:
        ESP_LOGI(TAG, "Security request received");
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;
    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        if (param->ble_security.auth_cmpl.success) {
            ESP_LOGI(TAG, "Authentication complete, bonded");
        } else {
            ESP_LOGE(TAG, "Authentication failed, status=%d, reason=%d", param->ble_security.auth_cmpl.success, param->ble_security.auth_cmpl.fail_reason);
        }
        break;
    default:
        break;
    }
}

esp_err_t hid_keyboard_init(void)
{
    esp_err_t ret;
    
    ESP_LOGI(TAG, "Initializing HID keyboard device");
    
    // Initialize BT controller
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "BT controller init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "BT controller enable failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize Bluedroid
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "Bluedroid init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Register callbacks
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret) {
        ESP_LOGE(TAG, "GAP callback register failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Set security parameters
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
    
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));
    
    // Initialize HID profile
    ret = esp_hidd_profile_init();
    if (ret) {
        ESP_LOGE(TAG, "HID profile init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Register HID callback
    ret = esp_hidd_register_callbacks(hidd_event_callback);
    if (ret) {
        ESP_LOGE(TAG, "HID callback register failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "HID keyboard initialized successfully");
    return ESP_OK;
}

esp_err_t hid_keyboard_press(uint8_t modifier, uint8_t key)
{
    if (!s_connected) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Send key with or without modifier
    if (key != 0) {
        uint8_t keys[1] = {key};
        esp_hidd_send_keyboard_value(0, (key_mask_t)modifier, keys, 1);
    } else {
        // Just modifier keys, no regular key
        esp_hidd_send_keyboard_value(0, (key_mask_t)modifier, NULL, 0);
    }
    
    return ESP_OK;
}

esp_err_t hid_keyboard_release(void)
{
    if (!s_connected) {
        return ESP_ERR_INVALID_STATE;
    }
    
    uint8_t buffer[1] = {0};
    esp_hidd_send_keyboard_value(0, (key_mask_t)0, buffer, 0);
    return ESP_OK;
}

esp_err_t hid_keyboard_media(uint16_t media_key)
{
    if (!s_connected) {
        return ESP_ERR_INVALID_STATE;
    }
    
    // Send media key press
    esp_hidd_send_consumer_value(0, media_key, true);
    vTaskDelay(pdMS_TO_TICKS(50));
    
    // Send media key release
    esp_hidd_send_consumer_value(0, 0, false);
    
    return ESP_OK;
}

bool hid_keyboard_is_connected(void)
{
    return s_connected;
}
