#include "WebUpdate.h"

WebUpdate ESPUpdate;
void buttonTriggerWrapper() { ESPUpdate.buttonTrigger(); }

void ICACHE_FLASH_ATTR WebUpdate::begin(void(*ptr)(), int updatePin)
{ 
  pinMode(updatePin, INPUT_PULLUP);     // MODEBUTTON as input for Config mode selection
  
  ESPhttpUpdate.rebootOnUpdate(false);
  
  updaterCallback = ptr;
  attachInterrupt(updatePin, buttonTriggerWrapper, CHANGE);
}

void WebUpdate::buttonTrigger()
{
  //debounce the button
  if (millis() - lastPressed > 250)
  {
    lastPressed = millis();
    updaterCallback();
  }
}

byte ICACHE_FLASH_ATTR WebUpdate::iotUpdaterSketch(String url, String firmware)
{
  byte retValue;
#ifdef DEBUG_UPDATER
  Serial.println(F("Updating Sketch from..."));
  Serial.print(F("   UPDATE_URL: "));
  Serial.println(url);
  Serial.print(F("   FIRMWARE_VERSION: "));
  Serial.println(firmware);
#endif
  
  t_httpUpdate_return ret = ESPhttpUpdate.update(url, firmware);
    
  switch (ret) 
  {
    case HTTP_UPDATE_FAILED:
    {
#ifdef DEBUG_UPDATER
      Serial.printf("SKETCH_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      Serial.println();
#endif
      retValue = 'F';
      break;
    }
    case HTTP_UPDATE_NO_UPDATES:
    {
#ifdef DEBUG_UPDATER
      Serial.println(F("---------- SKETCH_UPDATE_NO_UPDATES ------------------"));
#endif
      retValue = 'A';
      break;
    }
    case HTTP_UPDATE_OK:
    {
#ifdef DEBUG_UPDATER
      Serial.println(F("SKETCH_UPDATE_OK"));
#endif
      retValue = 'U';
      break;
    }
    default:
    {
#ifdef DEBUG_UPDATER
      Serial.println(F("Update Error"));
      Serial.printf("SKETCH_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
#endif
    }
  }
  return retValue;
}

byte ICACHE_FLASH_ATTR WebUpdate::iotUpdaterSPIFFS(String url, String firmware)
{
  byte retValue;
#ifdef DEBUG_UPDATER
  Serial.println(F("Updating SPIFFS from..."));
  Serial.print(F("UPDATE_URL: "));
  Serial.println(url);
  Serial.print(F("SPIFFS_VERSION: "));
  Serial.println(firmware);
#endif

  t_httpUpdate_return retspiffs = ESPhttpUpdate.updateSpiffs(url, firmware);
  
  switch (retspiffs) 
  {
    case HTTP_UPDATE_FAILED:
    {
#ifdef DEBUG_UPDATER
      Serial.printf("SPIFFS_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      Serial.println();
#endif
      retValue = 'F';
      break;
    }
    case HTTP_UPDATE_NO_UPDATES:
    {
#ifdef DEBUG_UPDATER
      Serial.println(F("---------- SPIFFS_UPDATE_NO_UPDATES ------------------"));
#endif
      retValue = 'A';
      break;
    }
    case HTTP_UPDATE_OK:
    {
#ifdef DEBUG_UPDATER
      Serial.println(F("SPIFFS_UPDATE_OK"));
#endif
      retValue = 'U';
      break;
    }
  }
  return retValue;
}
