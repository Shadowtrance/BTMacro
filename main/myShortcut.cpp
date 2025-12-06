#include "myShortcut.h"
#include "hidKeyboard.h"
#include "hidDev.h"

MyShortcut::MyShortcut(int caseId){
    shortcutId = caseId;
}

void MyShortcut::ReleaseAllKeys(){
    hid_keyboard_release();
}

void MyShortcut::Action(){
    switch(shortcutId){
      //Previous
      case 0:  
        hid_keyboard_media(HID_CONSUMER_SCAN_PREV_TRK);
      break;
  
       // Play / Pause
      case 1:  
        hid_keyboard_media(HID_CONSUMER_PLAY_PAUSE);
      break;
  
       //Next
      case 2:  
        hid_keyboard_media(HID_CONSUMER_SCAN_NEXT_TRK);
      break;
  
      //Mute
      case 3:  
        hid_keyboard_media(HID_CONSUMER_MUTE);
      break;
  
       // Volume Up
      case 4:  
        hid_keyboard_media(HID_CONSUMER_VOLUME_UP);
      break;
  
      //Volume Down
      case 5:  
        hid_keyboard_media(HID_CONSUMER_VOLUME_DOWN);
      break;
  
      // 1
      case 6:  
        hid_keyboard_press(0, HID_KEY_1);
      break;
  
      // 2
      case 7:  
        hid_keyboard_press(0, HID_KEY_2);
      break;

      // 3
      case 8:  
        hid_keyboard_press(0, HID_KEY_3);
      break;

      // 4
      case 9:  
        hid_keyboard_press(0, HID_KEY_4);
      break;

      // 5
      case 10:  
        hid_keyboard_press(0, HID_KEY_5);
      break;

      // 6
      case 11:  
        hid_keyboard_press(0, HID_KEY_6);
      break;
  
      default:
      break;
    }
}