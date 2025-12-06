#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize and start HID keyboard device
 */
esp_err_t hid_keyboard_init(void);

/**
 * @brief Send keyboard key press
 */
esp_err_t hid_keyboard_press(uint8_t modifier, uint8_t key);

/**
 * @brief Send keyboard key release
 */
esp_err_t hid_keyboard_release(void);

/**
 * @brief Send media key
 */
esp_err_t hid_keyboard_media(uint16_t media_key);

/**
 * @brief Check if device is connected
 */
bool hid_keyboard_is_connected(void);

#ifdef __cplusplus
}
#endif
