# BTMacro - Bluetooth HID Macro Keyboard

A Bluetooth Low Energy (BLE) HID keyboard implementation for ESP32-S3 with touchscreen interface. Features 15 programmable buttons for media controls and keyboard shortcuts.

## Hardware

- **Board**: ESP32-S3 (Sunton ESP32-S3-8048S043C)
- **Display**: 800x480 RGB LCD with GT911 touch controller
- **Bluetooth**: BLE 4.2 (Bluedroid stack)

## Features

- ✅ Full BLE HID keyboard functionality
- ✅ Keyboard shortcuts (Win+Tab, Win+D, Ctrl+Shift+Esc, Alt+F4)
- ✅ Media key support (Play/Pause, Volume, Track control)
- ✅ Number key support (1-6)
- ✅ 15-button touchscreen grid interface
- ✅ Secure pairing with bonding support
- ✅ 100% battery level reporting
- ✅ Works with Windows and Android

## Button Mappings

### Row 1
- **Button 1**: Previous Track
- **Button 2**: Play/Pause
- **Button 3**: Next Track
- **Button 4**: Mute
- **Button 5**: Number 0

### Row 2
- **Button 6**: Volume Up
- **Button 7**: Volume Down
- **Button 8**: Number 1
- **Button 9**: Number 2
- **Button 10**: Number 0

### Row 3
- **Button 11**: Number 3
- **Button 12**: Number 4
- **Button 13**: Number 5
- **Button 14**: Number 6
- **Button 15**: Number 0

## Prerequisites

- **ESP-IDF**: v5.5.1 or later

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

# Set environment variables (Windows PowerShell)
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
idf.py -p COM3 flash monitor
```
*Replace COM3 with your serial port*

## Configuration

### Bluetooth Settings
Key Bluetooth configurations in `sdkconfig.defaults`:
```
CONFIG_BT_ENABLED=y
CONFIG_BT_BLE_42_FEATURES_SUPPORTED=y
```

### Battery Level
To change the reported battery percentage, edit `main/hidDeviceLePrf.c`:
```c
static uint8_t battary_lev = 100;  // Line 295
```

### Customizing Button Actions
Edit the switch statement in `main/myShortcut.cpp` to customize button behaviors:
```cpp
void MyShortcut::Action(){
    switch(shortcutId){
        case 0:  
            hid_keyboard_media(HID_CONSUMER_SCAN_PREV_TRK);
            break;
        // Add your custom shortcuts here
    }
}
```

## Project Structure

```
BTMacro/
├── main/
│   ├── mainApp.cpp              # Main application entry point (CUSTOM)
│   ├── hidController.cpp/.h     # HID connection state management (CUSTOM)
│   ├── myShortcut.cpp/.h        # Button-to-action mappings (CUSTOM)
│   ├── hidKeyboard.c/.h         # HID keyboard API wrapper (CUSTOM)
│   ├── displayConfig.c/.h       # Display driver for Sunton board (CUSTOM)
│   ├── espHiddPrfApi.c/.h       # ESP-IDF HID profile API (FROM ESP-IDF EXAMPLE)
│   ├── hidDeviceLePrf.c         # GATT service implementation (FROM ESP-IDF EXAMPLE)
│   ├── hiddLePrfInt.h           # HID profile internals (FROM ESP-IDF EXAMPLE)
│   ├── hidDev.c/.h              # HID report utilities (FROM ESP-IDF EXAMPLE)
│   └── CMakeLists.txt
│   └── idf_component.yml
├── sdkconfig.defaults           # Default configuration
└── CMakeLists.txt
```

### File Origins

**Custom Files (Created for this project):**
- `mainApp.cpp` - Application entry, LVGL UI setup, button grid
- `hidController.cpp/.h` - High-level BLE connection state manager
- `myShortcut.cpp/.h` - Maps button presses to HID actions
- `hidKeyboard.c/.h` - Clean API wrapper around ESP-IDF HID profile
- `displayConfig.c/.h` - Sunton ESP32-S3 display initialization

**From ESP-IDF Example** (`examples/bluetooth/bluedroid/ble/ble_hid_device_demo`):
- `espHiddPrfApi.c/.h` - HID profile API (modified: renamed from `esp_hidd_prf_api`)
- `hidDeviceLePrf.c` - GATT services (modified: battery level changed to 100%)
- `hiddLePrfInt.h` - Internal profile structures (modified: renamed from `hidd_le_prf_int.h`)
- `hidDev.c/.h` - HID report descriptors and key codes (modified: renamed from `hid_dev`)

All ESP-IDF example files have been renamed to camelCase for consistency.

## Usage

### First Pairing
1. Flash the firmware and power on the device
2. On your computer/phone, search for Bluetooth devices
3. Connect to "Macro Deck"
4. The device will pair automatically (no PIN required)
5. Bonding information is saved for future connections

### Using Shortcuts
- Tap any button on the touchscreen to trigger the assigned action
- Connection status is tracked internally via the `HidController` class

## API Reference

### HID Keyboard API (`hidKeyboard.c`)
```c
esp_err_t hid_keyboard_init(void);
esp_err_t hid_keyboard_press(uint8_t modifier, uint8_t key);
esp_err_t hid_keyboard_release(void);
esp_err_t hid_keyboard_media(uint16_t media_key);
bool hid_keyboard_is_connected(void);
```

### Available Modifiers and Keys (`hidDev.h`)
```c
MOD_LEFT_CTRL   = 0x01
MOD_LEFT_SHIFT  = 0x02
MOD_LEFT_ALT    = 0x04
MOD_LEFT_GUI    = 0x08  // Windows/Command key
```

### Example: Custom Shortcut
```cpp
// Ctrl + C (Copy)
hid_keyboard_press(MOD_LEFT_CTRL, HID_KEY_C);
hid_keyboard_release();

// Alt + Tab
hid_keyboard_press(MOD_LEFT_ALT, HID_KEY_TAB);
hid_keyboard_release();
```

## Troubleshooting

### Device won't pair
- Ensure Bluetooth is enabled on your host device
- Try removing old pairings: Settings → Bluetooth → Remove "ESP32 BLE HID Keyboard"
- Check logs with `idf.py monitor`

### Buttons not responding
- Verify connection status on display
- Check serial monitor for error messages
- Ensure device is paired and connected

### Build errors
- Verify ESP-IDF version: `idf.py --version` (should be v5.5.1+)
- Clean build: `idf.py fullclean && idf.py build`
- Check that managed components are installed

## Credits

- ESP-IDF BLE HID Device Example (Espressif Systems)
- LVGL Graphics Library
- Sunton ESP32-S3 Display Driver

---

**Note**: This project requires the official ESP-IDF environment and cannot be built with Arduino IDE.
