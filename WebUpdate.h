#pragma once

//#define DEBUG_UPDATER

#include <ESP8266WiFi.h>
#include <ESP8266httpUpdate.h>

class WebUpdate {
  public:
    void ICACHE_FLASH_ATTR begin(void(*ptr)(), int updatePin);
    void buttonTrigger();
    byte ICACHE_FLASH_ATTR iotUpdaterSketch(String url, String firmware);
    byte ICACHE_FLASH_ATTR iotUpdaterSPIFFS(String url, String firmware);
  private:
    void(*updaterCallback)();
    unsigned long lastPressed = 0L;
};
extern WebUpdate ESPUpdate;
