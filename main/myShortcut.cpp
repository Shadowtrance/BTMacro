#include "myShortcut.h"
#include "hidKeyboard.h"
#include "hidDev.h"
#include "sdCard.h"
#include "hidCodes.h"
#include "helpers.h"
#include <cJSON.h>
#include <map>
#include <string>
#include <vector>
#include <cstdio>
#include <esp_log.h>

static const char *TAG = "[SHORTCUTS]";

std::map<int, std::vector<ShortcutAction>> MyShortcut::shortcuts;
bool MyShortcut::loaded = false;

void MyShortcut::loadDefaults() {
    shortcuts[0] = {{ShortcutAction{"keyboard", "HID_KEY_0", 0}}};
    shortcuts[1] = {{ShortcutAction{"keyboard", "HID_KEY_1", 0}}};
    shortcuts[2] = {{ShortcutAction{"keyboard", "HID_KEY_2", 0}}};
    shortcuts[3] = {{ShortcutAction{"keyboard", "HID_KEY_3", 0}}};
    shortcuts[4] = {{ShortcutAction{"keyboard", "HID_KEY_4", 0}}};
    shortcuts[5] = {{ShortcutAction{"keyboard", "HID_KEY_5", 0}}};
}

MyShortcut::MyShortcut(int caseId){
    shortcutId = caseId;
}

void MyShortcut::ReleaseAllKeys(){
    hid_keyboard_release();
}

void MyShortcut::loadShortcuts() {
    char path[64];
    snprintf(path, sizeof(path), "%s/shortcuts.json", MOUNT_POINT);
    cJSON *root = load_json_from_file(path);
    if (!root) {
        ESP_LOGI(TAG, "Shortcuts JSON not available or invalid, using defaults");
        return;
    }
  
    cJSON *shortcutsArray = cJSON_GetObjectItem(root, "shortcuts");
    if (!cJSON_IsArray(shortcutsArray)) {
        ESP_LOGE(TAG, "Invalid JSON structure, using defaults");
        cJSON_Delete(root);
        return;
    }
  
    cJSON *shortcut = NULL;
    cJSON_ArrayForEach(shortcut, shortcutsArray) {
        int id = cJSON_GetObjectItem(shortcut, "id")->valueint;
        cJSON *actions = cJSON_GetObjectItem(shortcut, "actions");
        std::vector<ShortcutAction> acts;
        cJSON *action = NULL;
        cJSON_ArrayForEach(action, actions) {
            ShortcutAction act;
            act.type = cJSON_GetObjectItem(action, "type")->valuestring;
            if (act.type == "media") {
                act.code = cJSON_GetObjectItem(action, "code")->valuestring;
            } else if (act.type == "keyboard") {
                // Parse modifiers
                uint8_t mod = 0;
                cJSON *modifiers = cJSON_GetObjectItem(action, "modifiers");
                if (cJSON_IsArray(modifiers)) {
                    cJSON *modItem = NULL;
                    cJSON_ArrayForEach(modItem, modifiers) {
                        if (cJSON_IsString(modItem)) {
                            std::string modStr = modItem->valuestring;
                            auto it = modMap.find(modStr);
                            if (it != modMap.end()) {
                                mod |= it->second;
                            } else {
                                ESP_LOGE(TAG, "Unknown modifier: %s", modStr.c_str());
                            }
                        }
                    }
                } else {
                    // Fallback to old "modifier" as int or array
                    cJSON *modItem = cJSON_GetObjectItem(action, "modifier");
                    if (cJSON_IsArray(modItem)) {
                        // Treat as array of strings
                        cJSON *arrItem = NULL;
                        cJSON_ArrayForEach(arrItem, modItem) {
                            if (cJSON_IsString(arrItem)) {
                                std::string modStr = arrItem->valuestring;
                                auto it = modMap.find(modStr);
                                if (it != modMap.end()) {
                                    mod |= it->second;
                                } else {
                                    ESP_LOGE(TAG, "Unknown modifier: %s", modStr.c_str());
                                }
                            }
                        }
                    } else if (cJSON_IsNumber(modItem)) {
                        mod = modItem->valueint;
                    }
                }
                act.modifier = mod;
                act.code = cJSON_GetObjectItem(action, "key")->valuestring;
            }
            acts.push_back(act);
        }
        shortcuts[id] = acts;
    }
    cJSON_Delete(root);
    loaded = true;
    ESP_LOGI(TAG, "Successfully loaded shortcuts from %s", path);
}
  
void MyShortcut::Action(){
    if (!loaded) {
        loadShortcuts();
        if (!loaded) {
            loadDefaults();
            loaded = true;
        }
    }
    auto it = shortcuts.find(shortcutId);
    if (it == shortcuts.end()) return;
    for (const auto& act : it->second) {
        auto codeIt = allCodes.find(act.code);
        if (codeIt == allCodes.end()) {
            ESP_LOGE(TAG, "Unknown HID code: %s", act.code.c_str());
            continue;
        }
        int hidCode = codeIt->second;
        if (act.type == "media") {
            hid_keyboard_media(hidCode);
        } else if (act.type == "keyboard") {
            hid_keyboard_press(act.modifier, hidCode);
        }
    }
    }
