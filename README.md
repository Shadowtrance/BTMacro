# BTMacro - Bluetooth HID Macro Keyboard

A Bluetooth Low Energy (BLE) HID keyboard implementation for ESP32-S3 with dynamic touchscreen interface. Features programmable buttons for media controls, keyboard shortcuts, and custom actions via SD card JSON configuration.

## Hardware

- **Board**: ESP32-S3 (Sunton ESP32-S3-8048S043C or compatible)
- **Display**: 800x480 RGB LCD with GT911 touch controller
- **Touchscreen**: Capacitive I2C GT911
- **SD Card**: SPI-connected microSD slot for configuration files
- **Bluetooth**: BLE 4.2 (Bluedroid stack)

## Features

- ✅ Full BLE HID keyboard (including F1-F24) and media control functionality
- ✅ Dynamic touchscreen grid (configurable via SD card JSON)
- ✅ Programmable shortcuts (keyboard combos, media keys, custom actions)
- ✅ Secure pairing with bonding support
- ✅ 100% battery level reporting
- ✅ SD card-based configuration with fallback to defaults
- ✅ Modular C/C++ codebase
- ✅ Works with Windows, Android, and other BLE hosts
- ✅ LVGL-powered UI for responsive touch interaction

## Prerequisites

- **ESP-IDF**: v5.5.1 or later
- **MicroSD Card** (optional, for custom configs): FAT32 formatted
- **Serial Programmer**: For flashing (e.g., USB-to-UART)

## Installation

### 1. Clone the repository
```bash
git clone https://github.com/Shadowtrance/BTMacro.git
cd BTMacro
```

### 2. Set up ESP-IDF environment
```bash
# Install ESP-IDF v5.5.1
# Follow: https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/

# Windows PowerShell
. $env:IDF_PATH/export.ps1

# Linux/macOS
. $IDF_PATH/export.sh
```

### 3. Build the project
```bash
idf.py build
```

### 4. Flash to ESP32-S3
```bash
idf.py -p COM3 flash
```
*Replace COM3 with your serial port*

### 5. Monitor (optional)
```bash
idf.py monitor
```

## Configuration

### SD Card Setup (Optional)
Insert a microSD card with the following JSON files in the root:

- **`button_map.json`**: Defines the UI grid layout and button labels.
- **`shortcuts.json`**: Maps buttons to HID actions (keyboard, media, etc.).

If no SD card or files are present, the device uses built-in defaults (6-button grid with basic actions).

#### Example `button_map.json`
```json
{
  "buttons": [
    {"id": 0, "label": "\\xEF\\x81\\x88"},
    {"id": 1, "label": "\\xEF\\x81\\x8B"},
    {"id": 2, "label": "\\xEF\\x81\\x91"},
    {"id": 3, "label": "\\xEF\\x81\\xA7"},
    {"id": 4, "label": "\\xEF\\x81\\xA8"},
    {"id": 5, "label": "\\xEF\\x84\\x9C"}
  ]
}
```

#### Example `shortcuts.json`
```json
{
  "shortcuts": [
    {"id": 0, "actions": [{"type": "media", "code": "HID_CONSUMER_SCAN_PREV_TRK"}]},
    {"id": 1, "actions": [{"type": "media", "code": "HID_CONSUMER_PLAY_PAUSE"}]},
    {"id": 2, "actions": [{"type": "media", "code": "HID_CONSUMER_SCAN_NEXT_TRK"}]},
    {"id": 3, "actions": [{"type": "media", "code": "HID_CONSUMER_VOLUME_UP"}]},
    {"id": 4, "actions": [{"type": "media", "code": "HID_CONSUMER_VOLUME_DOWN"}]},
    {"id": 5, "actions": [{"type": "media", "code": "HID_CONSUMER_MUTE"}]},
    {"id": 6, "actions": [{"type": "keyboard", "modifiers": ["MOD_LEFT_GUI"], "key": "HID_KEY_TAB"}]}
  ]
}
```

### JSON Structure

#### `button_map.json`
Defines the touchscreen button grid layout.

- **Root Object**:
  - `buttons`: Array of button objects or strings (for backward compatibility).

- **Button Objects**:
  - `id` (integer, optional): Button ID (for reference, not required for functionality).
  - `label` (string): Button text/icon. Supports escaped UTF-8 (e.g., `"\\xEF\\x81\\x88"` for LVGL icons) and `\n` for newlines.

Dynamic grid supporting up to 15 buttons: 3 columns for ≤9 buttons, 4 columns for 10-12, 5 columns for 13-15, with automatic row breaks.

#### `shortcuts.json`
Maps button IDs to HID actions (keyboard shortcuts, media keys, etc.).

- **Root Object**:
  - `shortcuts`: Array of shortcut objects.

- **Shortcut Object**:
  - `id` (integer): Button ID this shortcut applies to.
  - `actions`: Array of action objects (executed in order).

- **Action Object**:
  - `type` (string): Action type (`"keyboard"` or `"media"`).
  - For `type: "keyboard"`:
    - `modifiers` (array of strings, optional): Modifier keys (e.g., `["MOD_LEFT_SHIFT", "MOD_LEFT_GUI"]`). Use empty array or omit for none.
    - `key` (string): Key code (e.g., `"HID_KEY_A"`).
    - `modifier` (integer, deprecated): Old format for single/combined modifiers (e.g., 10 for LEFT_SHIFT | LEFT_GUI). Supported for backward compatibility.
  - For `type: "media"`:
    - `code` (string): Media key code (e.g., `"HID_CONSUMER_PLAY_PAUSE"`).

Examples:
- Keyboard with no modifiers: `{"type": "keyboard", "modifiers": [], "key": "HID_KEY_A"}`
- Keyboard with single modifier: `{"type": "keyboard", "modifiers": ["MOD_LEFT_CTRL"], "key": "HID_KEY_C"}`
- Keyboard with multiple modifiers: `{"type": "keyboard", "modifiers": ["MOD_LEFT_SHIFT", "MOD_LEFT_GUI"], "key": "HID_KEY_RIGHT_ARROW"}`
- Media: `{"type": "media", "code": "HID_CONSUMER_PLAY_PAUSE"}`

Available codes are defined in `hidCodes.h` (keyboard keys, modifiers, consumer keys).

### Bluetooth Settings
Key configurations in `sdkconfig.defaults`:
```
CONFIG_BT_ENABLED=y
CONFIG_BT_BLE_42_FEATURES_SUPPORTED=y
```

### Battery Level
To change the reported battery percentage, edit `main/hidDeviceLePrf.c`:
```c
static uint8_t battary_lev = 100;  // Line ~295
```

## Project Structure

```
BTMacro/
├── main/
│   ├── mainApp.cpp              # Main application entry point
│   ├── hidController.cpp/.h     # HID connection state management
│   ├── ui.cpp/.h                # LVGL UI grid and button handling
│   ├── helpers.cpp/.h           # JSON parsing utilities
│   ├── sdCard.cpp/.h            # SD card initialization and mounting
│   ├── msgQueue.cpp/.h          # Button press message queue
│   ├── myShortcut.cpp/.h        # Dynamic shortcut mapping from JSON
│   ├── hidCodes.cpp/.h          # HID key code mappings
│   ├── hidKeyboard.c/.h         # HID keyboard API wrapper
│   ├── displayConfig.c/.h       # Display and touchscreen driver
│   ├── espHiddPrfApi.c/.h       # ESP-IDF HID profile API
│   ├── hidDeviceLePrf.c         # GATT service implementation
│   ├── hiddLePrfInt.h           # HID profile internals
│   ├── hidDev.c/.h              # HID report utilities
│   ├── CMakeLists.txt
│   └── idf_component.yml
├── sdkconfig.defaults           # Default ESP-IDF configuration
├── CMakeLists.txt
├── partitions.csv               # Partition table
├── button_map.json              # Default UI grid config
├── shortcuts.json               # Default shortcuts config
└── README.md
```

### File Origins

**Custom Files (Created/Modified for this project):**
- `mainApp.cpp` - App initialization, SD/display setup, task management
- `hidController.cpp/.h` - BLE connection state and HID report sending
- `ui.cpp/.h` - LVGL touchscreen grid creation and event handling
- `helpers.cpp/.h` - JSON loading and parsing for configs
- `sdCard.cpp/.h` - SPI SD card mount/verification
- `msgQueue.cpp/.h` - FreeRTOS queue for button events
- `myShortcut.cpp/.h` - Dynamic action mapping with fallbacks
- `hidCodes.cpp/.h` - Centralized HID key/media code definitions
- `hidKeyboard.c/.h` - Wrapper for ESP-IDF HID functions
- `displayConfig.c/.h` - Sunton board LCD/touch init

**From ESP-IDF Example** (`examples/bluetooth/bluedroid/ble/ble_hid_device_demo`):
- `espHiddPrfApi.c/.h` - HID profile API (renamed, minor mods)
- `hidDeviceLePrf.c` - GATT services (battery level set to 100%)
- `hiddLePrfInt.h` - Internal structures (renamed)
- `hidDev.c/.h` - HID descriptors and codes (renamed)

## Usage

### First Pairing
1. Insert SD card (optional) and power on the device
2. On your host device, enable Bluetooth and search for devices
3. Connect to "Macro Deck"
4. Pairing completes automatically (no PIN, bonding enabled)
5. UI loads from SD card or defaults

### Using the Device
- Tap buttons on the touchscreen to trigger actions
- Connection status is indicated via internal state
- Reconnect automatically after disconnections

### Customizing
- Edit JSON files on SD card for custom grids/shortcuts
- Reboot or reinsert SD to reload configs
- Use `hidCodes.h` for available HID codes

## API Reference

### HID Keyboard API (`hidKeyboard.c`)
```c
esp_err_t hid_keyboard_init(void);
esp_err_t hid_keyboard_press(uint8_t modifier, uint8_t key);
esp_err_t hid_keyboard_release(void);
esp_err_t hid_keyboard_media(uint16_t media_key);
bool hid_keyboard_is_connected(void);
```

### Modifiers and Keys (`hidCodes.h`)
```c
// Modifiers
MOD_LEFT_CTRL   = 0x01
MOD_LEFT_SHIFT  = 0x02
MOD_LEFT_ALT    = 0x04
MOD_LEFT_GUI    = 0x08  // Windows/Command

// Keys: HID_KEY_A, HID_KEY_B, ..., HID_KEY_F12, HID_KEY_F13, ..., HID_KEY_F24, etc.
// Media: HID_CONSUMER_PLAY, HID_CONSUMER_VOLUME_UP, etc.
```

### Example: Custom Shortcut
```cpp
// Ctrl + C
hid_keyboard_press(MOD_LEFT_CTRL, HID_KEY_C);
hid_keyboard_release();

// Shift + GUI + Right Arrow (multi-key combo)
hid_keyboard_press(MOD_LEFT_SHIFT | MOD_LEFT_GUI, HID_KEY_RIGHT_ARROW);
hid_keyboard_release();

// Media Play
hid_keyboard_media(HID_CONSUMER_PLAY_PAUSE);
```

## Troubleshooting

### Device won't pair
- Forget old pairings on host device
- Check `idf.py monitor` for logs
- Ensure no interference (crowded 2.4GHz)

### Buttons not responding
- Verify touchscreen calibration (Y-mirror in `displayConfig.c`)
- Check SD card for valid JSON
- Monitor for HID report errors

### SD card not loading
- Ensure FAT32 format
- Check SPI pins in `displayConfig.h`
- Falls back to defaults if issues

### Build errors
- Confirm ESP-IDF v5.5.1+
- Run `idf.py fullclean`
- Check component dependencies

## Credits

- ESP-IDF BLE HID Example (Espressif Systems)
- LVGL Graphics Library
- cJSON for JSON parsing
- Sunton ESP32-S3 Board Documentation

---

**Note**: Requires ESP-IDF environment. Not compatible with Arduino IDE.
