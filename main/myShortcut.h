#pragma once

#include <vector>
#include <string>
#include <map>

struct ShortcutAction {
    std::string type; // "media" or "keyboard"
    std::string code; // HID code name, e.g., "HID_KEY_A"
    int modifier; // for keyboard, modifier keys
};

class MyShortcut
{
  private:
    int shortcutId;
    static std::map<int, std::vector<ShortcutAction>> shortcuts;
    static bool loaded;

  public:
    MyShortcut(int caseId);
    void Action();
    void ReleaseAllKeys();
    static void loadShortcuts();
    static void loadDefaults();
};
