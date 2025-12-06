#pragma once

class HidController
{
  private:
    void(*callback)(bool);
    unsigned long previousTimestamp;
    friend void sendTask(void* pvParameters);
    
  public:
    HidController();
    bool isConnected;
    void begin();
    void loop();
    void triggerTask(int caseNum);
    void bindCallback(void(*fn)(bool)) {callback = fn;}
};
