/* This project was based on the esp-mqtt-rgb-led project by Corban Mailloux:
 *  https://github.com/corbanmailloux/esp-mqtt-rgb-led
 */

/*
   v0.1.0 - Removed IOTAppStory and added HTML web status and config page
   v0.1.1 - Memory leak fix (?), random bug fixes, added random color on turn on if no color/transistion defined
    *** Don't update to ESP Board 2.4.2  until confirm OTA is fixed
    ***   https://github.com/esp8266/Arduino/issues/5023
*/

#include <umm_malloc/umm_malloc.h>
const size_t block_size = 8;
size_t getTotalAvailableMemory();
size_t getLargestAvailableBlock();
inline float getFragmentation();

#include <ESP8266WiFi.h>
#include <ESP8266TrueRandom.h>

#include "global.h"

#ifdef DEBUG_MEMORY
  uint32_t lastMem = 0;
  unsigned long lastMemCheck = 0L;
#endif

// https://github.com/bblanchon/ArduinoJson
#include <ArduinoJson.h>

#ifdef ENABLE_MQTT
  // http://pubsubclient.knolleary.net/
  // https://pubsubclient.knolleary.net/api.html
  
  void mqttCallback(char* topic, byte* payload, unsigned int length);
  
  bool ENABLE_SEND_STATE = true; 
#endif //ENABLE_MQTT

#include "mqtt_esp8266_light.h"
#include "SPIFFS_Params.h"
#include "ServerSide.h"
#include "WebUpdate.h"

#ifdef ENABLE_NTP
  #include "NTP.h"
  NtpClient NTP;
#endif //ENABLE_NTP

#ifdef ENABLE_TEMPERATURE_SENSOR
  #include <OneWire.h>
  #include <DallasTemperature.h>
  #define ONE_WIRE_BUS 5  // DS18B20 pin
  OneWire oneWire(ONE_WIRE_BUS);
  DallasTemperature tempSensor(&oneWire);
#endif

void setup()
{
#ifdef ENABLE_DEBUG
  Serial.begin(115200);
#endif
  
  if (rgb)
  {
    pinMode(CONFIG_PIN_RED, OUTPUT);
    pinMode(CONFIG_PIN_GREEN, OUTPUT);
    pinMode(CONFIG_PIN_BLUE, OUTPUT);
    digitalWrite(CONFIG_PIN_RED, LOW);
    digitalWrite(CONFIG_PIN_GREEN, LOW);
    digitalWrite(CONFIG_PIN_BLUE, LOW);
  }
  if (includeWhite)
  {
    pinMode(CONFIG_PIN_WHITE, OUTPUT);
    digitalWrite(CONFIG_PIN_WHITE, LOW);
  }
  
  //read SPIFF for values
  //if SPIFF file isn't found, use default values
  beginSPIFFS();
  readSPIFFS_Settings();
  
  //start WiFi
  connectNetwork();
  ESPUpdate.begin(&updateMyESP, MODEBUTTON);

  
  // Set the BUILTIN_LED based on the CONFIG_BUILTIN_LED_MODE
  switch (CONFIG_BUILTIN_LED_MODE) {
    case 0:
      pinMode(BUILTIN_LED, OUTPUT);
      digitalWrite(BUILTIN_LED, LOW);
      break;
    case 1:
      pinMode(BUILTIN_LED, OUTPUT);
      digitalWrite(BUILTIN_LED, HIGH);
      break;
    default: // Other options (like -1) are ignored.
      break;
  }
  
  analogWriteRange(255);
  
#ifdef ENABLE_MQTT
  // Setup MQTT
  client.setServer(settings.mqtt.CONFIG_MQTT_HOST, settings.mqtt.CONFIG_MQTT_PORT);
  client.setCallback(mqttCallback);
#endif //ENABLE_MQTT
  
  current_transition = OFF;
  previous_transition = OFF;
  
#ifdef ENABLE_NTP
//  NTP.Ntp(settings.ntp.ntpserver, settings.ntp.timeZone, settings.ntp.ntp_interval * 60);
  NTP.Ntp(settings.ntp.ntpserver, settings.ntp.timeZone);
#endif // ENABLE_NTP
  
  beginServerServices();
}

void updateMyESP()
{
  checkForUpdate = true;
}

/*
  SAMPLE PAYLOAD (BRIGHTNESS):
  {
    "brightness": 120,
    "flash": 2,
    "transition": 5,
    "state": "ON"
    "temp": xy.z
  }

  SAMPLE PAYLOAD (RGBW):
  {
    "brightness": 120,
    "color": {
      "r": 255,
      "g": 100,
      "b": 100
    },
    "white_value": 255,
    "flash": 2,
    "transition": 5,
    "state": "ON",
    "effect": "colorfade_fast",
    "temp": xy.z
  }
*/
#ifdef ENABLE_MQTT
void mqttCallback(char* topic, byte* payload, unsigned int length)
{
  payload[length]='\0';

#ifdef DEBUG_MQTT
  Serial.print(F("Message arrived ["));
  Serial.print(F(": "));
  Serial.print(topic);
  Serial.println(F("] "));
  Serial.print(F("-    "));
  Serial.println((char*)payload);
#endif

  StaticJsonBuffer<256> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(payload);

  if (!root.success())
  {
#ifdef DEBUG_MQTT
    Serial.println(F("parseObject() failed"));
#endif
    return;
  }

  if (root.containsKey(F("state")))
  {
    if (strcmp(root[F("state")], CONFIG_MQTT_PAYLOAD_ON) == 0)
    {
      if (stateOn == false) //only save the transition if
        current_transition = previous_transition;

      lastState = stateOn;
      stateOn = true;
    }
    else if (strcmp(root[F("state")], CONFIG_MQTT_PAYLOAD_OFF) == 0)
    {
      previous_transition = current_transition;
      current_transition = OFF;
      
      lastState = stateOn;
      stateOn = false;
    }
  }

  // If "flash" is included, treat RGB and brightness differently
  if (root.containsKey(F("flash")) || (root.containsKey(F("effect")) && strcmp(root[F("effect")], "flash") == 0))
  {
    if (root.containsKey(F("flashlength")))
    {
      settings.transitions.CONFIG_FLASH_LENGTH = (int)root[F("flashlength")] * 1000;
    }

    if (root.containsKey(F("numflashes")))
    {
      settings.transitions.CONFIG_NUM_FLASHS = (int)root[F("numflashes")];
    }

    if (!root.containsKey(F("brightness")))
    {
      flashBrightness = brightness;
    }

    if (rgb && root.containsKey(F("color")))
    {
      flashRed = root[F("color")][F("r")];
      flashGreen = root[F("color")][F("g")];
      flashBlue = root[F("color")][F("b")];
    }
    else
    {
      flashRed = red;
      flashGreen = green;
      flashBlue = blue;
    }

    if (includeWhite && root.containsKey(F("white_value")))
    {
      flashWhite = root[F("white_value")];
    }
    else
    {
      flashWhite = white;
    }

    flashRed = map(flashRed, 0, 255, 0, flashBrightness);
    flashGreen = map(flashGreen, 0, 255, 0, flashBrightness);
    flashBlue = map(flashBlue, 0, 255, 0, flashBrightness);
    flashWhite = map(flashWhite, 0, 255, 0, flashBrightness);

    previous_transition = current_transition;
    current_transition = FLASH;
    flash = true;
    startFlash = true;
  }
  else
  {
    if (root.containsKey(F("brightness")))
      brightness = root[F("brightness")];

    if (rgb && root.containsKey(F("effect")) && (strcmp(root[F("effect")], "rainbow") == 0))
    {
      flash = false;
      colorfade = true;
      currentColor = 0;
      transition_step = 0;
      current_transition = RAINBOW;

      red = ESP8266TrueRandom.randomByte();
      green = ESP8266TrueRandom.randomByte();
      blue = ESP8266TrueRandom.randomByte();

      lastUpdate = millis();
    }
    else if (rgb && root.containsKey(F("effect")) && (strcmp(root[F("effect")], "randomfade") == 0))
    {
      flash = false;
      colorfade = true;
      currentColor = 0;
      transition_step = 0;
      red = ESP8266TrueRandom.randomByte();
      green = ESP8266TrueRandom.randomByte();
      blue = ESP8266TrueRandom.randomByte();
      current_transition = RANDOM_FADE;
      lastUpdate = millis();
    }
    else if (rgb && root.containsKey(F("effect")) && (strcmp(root[F("effect")], "fade") == 0))
    {
      flash = false;
      colorfade = true;
      transition_step = 0;
      current_transition = FADE;
      lastUpdate = millis();
    }
    else
    {
      if (root.containsKey(F("color")) || root.containsKey(F("white_value")))
      {
        // No effect
        flash = false;
        colorfade = false;
        current_transition = SOLID;
      }

      if (rgb && root.containsKey(F("color")))
      {
        red = root[F("color")][F("r")];
        green = root[F("color")][F("g")];
        blue = root[F("color")][F("b")];
      }

      if (includeWhite && root.containsKey(F("white_value")))
      {
        white = root[F("white_value")];
      }
    }
  }

  if (stateOn)
  {
    if (red + green + blue == 0 && current_transition == OFF)
    {
      red = ESP8266TrueRandom.randomByte();
      green = ESP8266TrueRandom.randomByte();
      blue = ESP8266TrueRandom.randomByte();
      
      realRed = map(red, 0, 255, 0, brightness);
      realGreen = map(green, 0, 255, 0, brightness);
      realBlue = map(blue, 0, 255, 0, brightness);
      
      current_transition = SOLID;
    }
  }
  else
  {
    realRed = 0;
    realGreen = 0;
    realBlue = 0;
    realWhite = 0;
  }

  startFade = true;
  inFade = false; // Kill the current fade

  sendState();
}

void sendState()
{
  char buffer[256];

  StaticJsonBuffer<256> stateBuffer;
  JsonObject& statePacket = stateBuffer.createObject();

  statePacket[F("state")] = (stateOn) ? CONFIG_MQTT_PAYLOAD_ON : CONFIG_MQTT_PAYLOAD_OFF;
  
  if (stateOn)
  { 
    if (includeWhite)
    {
      statePacket[F("white_value")] = white;
    }
  
    statePacket[F("brightness")] = brightness;

    switch (current_transition)
    {
      case OFF: {
          statePacket[F("effect")] = F("off");
          break;
      }
      case SOLID: {
          statePacket[F("effect")] = F("solid");
          break;
      }
      case FADE: {
          statePacket[F("effect")] = F("fade");
          break;
        }
      case RANDOM_FADE: {
          statePacket[F("effect")] = F("randomfade");
          break;
        }
      case FLASH: {
          statePacket[F("effect")] = F("flash");
          break;
        }
      case RAINBOW: {
          statePacket[F("effect")] = F("rainbow");
          break;
        }
      default: {
          statePacket[F("effect")] = F("null");
        }
    }
    
    if (rgb)
    { 
      JsonObject& color = statePacket.createNestedObject(F("color"));
      color[F("r")] = red;
      color[F("g")] = green;
      color[F("b")] = blue;
    }
  }

  statePacket.printTo(buffer);

#ifdef DEBUG_STATE
  //Serial.print(F("Sending State: "));
  //Serial.println(buffer);
#endif //DEBUG_STATE

  client.publish(settings.mqtt.CONFIG_MQTT_TOPIC_STATE, buffer, false);

#ifdef ENABLE_TEMPERATURE_SENSOR
  if (sendTempStatusToMQTT >= 2) // sent every TEMP_UPDATE_INTERVAL * 2
  {
    StaticJsonBuffer<32> tempBuffer;
  
    JsonObject& tempState = tempBuffer.createObject();
    tempState[F("temp")] = temperature;
  
    tempState.printTo(buffer);
    client.publish(settings.mqtt.CONFIG_MQTT_TOPIC_TEMP, buffer, false);

    sendTempStatusToMQTT = 0;
  }
#endif//ENABLE_TEMPERATURE_SENSOR
}

void ICACHE_FLASH_ATTR reconnect()
{
#ifdef DEBUG_MQTT
  Serial.print(F("Attempting MQTT connection..."));
#endif

  bool connectedSuccessfully = false;
  
  // Attempt to connect
  if (!settings.mqtt.MQTT_USE_AUTHENTICATION || strlen(settings.mqtt.CONFIG_MQTT_USER) == 0)
    connectedSuccessfully = client.connect( settings.mqtt.CONFIG_MQTT_CLIENT_ID, settings.mqtt.CONFIG_MQTT_TOPIC_WILL, 0, true, "Offline" );
  else
    connectedSuccessfully = client.connect( settings.mqtt.CONFIG_MQTT_CLIENT_ID, settings.mqtt.CONFIG_MQTT_USER, settings.mqtt.CONFIG_MQTT_PASS, settings.mqtt.CONFIG_MQTT_TOPIC_WILL, 0, true, "Offline" );
  
  if (connectedSuccessfully)
  {
    client.subscribe(settings.mqtt.CONFIG_MQTT_TOPIC_SET);
    client.publish(settings.mqtt.CONFIG_MQTT_TOPIC_WILL, "Online", true);
    
#ifdef DEBUG_MQTT
    Serial.println(F("connected"));
#endif
  }
  else
  {
#ifdef DEBUG_MQTT
    Serial.println(F("failed"));
#endif

    client.unsubscribe(settings.mqtt.CONFIG_MQTT_TOPIC_SET);
    client.disconnect();
  }
}
#endif //ENABLE_MQTT

void loop()
{
  if (!inAPMode && WiFi.status() != WL_CONNECTED)
  {
    connectNetwork();
  }

#ifdef ENABLE_WEBSOCKET_SERVER
  webSocket.loop();
#endif //ENABLE_WEBSOCKET_SERVER

  server.handleClient();

#ifdef ENABLE_MQTT
  //check if connected to MQTT server
  if (WiFi.status() == WL_CONNECTED && !client.connected() && millis() - lastMQTTAttempt > MQTT_RECONNECT_INTERVAL )
  {
    reconnect();
    lastMQTTAttempt = millis();
  }

  client.loop();
#endif

#ifdef ENABLE_NTP
  //check NTP uptime to see if it's time to check for update (don't update more than once a minute)
  deviceUptimeSec = NTP.getUptimeSec();
#else
  deviceUptimeSec = millis() / 1000;
#endif

  if (checkForUpdate == true || stateOn == false && settings.ota.autoUpdateIntervalSeconds > 60 && deviceUptimeSec > timesCheckedUpdatesSinceReboot * settings.ota.autoUpdateIntervalSeconds)
  {
    checkForUpdate = false;
    timesCheckedUpdatesSinceReboot++;
    checkForUpdates();
  }

  //check NTP uptime to see if it's time to reboot the device (only reboot if device if stateOn == false and once only a minute)
  if (stateOn == false && settings.general.autoRestartIntervalSeconds > 60 && deviceUptimeSec > settings.general.autoRestartIntervalSeconds)
    ESP.restart();

#ifdef ENABLE_TEMPERATURE_SENSOR
  if (millis() - lastTempUpdate >= TEMP_UPDATE_INTERVAL)  //update temp every 5 seconds
  {
    //get the temperature of device
    getTemp();
    sendTempStatusToMQTT++;
    lastTempUpdate = millis();
  }
#endif

  if (stateOn)
  {
    switch (current_transition)
    {
      case SOLID: {
          //Serial.println(F("Set Solid Color"));
          realRed = map(red, 0, 255, 0, brightness);
          realGreen = map(green, 0, 255, 0, brightness);
          realBlue = map(blue, 0, 255, 0, brightness);
          realWhite = map(white, 0, 255, 0, brightness);
          
          setColor(realRed, realGreen, realBlue, realWhite);
          break;
        }
      case FLASH: {
          if (startFlash)
          {
            startFlash = false;
            flashCount = 0;
            flashStartTime = millis();
          }

          if (flashCount < settings.transitions.CONFIG_NUM_FLASHS)
          {
            if ((millis() - flashStartTime) % settings.transitions.CONFIG_FLASH_LENGTH <= (settings.transitions.CONFIG_FLASH_LENGTH / 2))
            {
              setColor(flashRed, flashGreen, flashBlue, flashWhite);
            }
            else
            {
              setColor(0, 0, 0, 0);
              // If you'd prefer the flashing to happen "on top of"
              // the current color, uncomment the next line.
              // setColor(realRed, realGreen, realBlue, realWhite);

              flashCount++;
            }
          }
          else
          {
            flash = false;
            setColor(realRed, realGreen, realBlue, realWhite);

            current_transition = previous_transition;
            if (current_transition == OFF)
            {
              stateOn = false;
            }
          }
          break;
        }
      case FADE: {
          if (millis() - lastUpdate > settings.transitions.fadeRate)
          {
            colorFadeEffect(transition_step++);
            lastUpdate = millis();
          }
          break;
        }
      case RANDOM_FADE: {
          if (millis() - lastUpdate > settings.transitions.randFadeRate)
          {
            colorFadeRotateEffect(transition_step++);
            lastUpdate = millis();
          }
          break;
        }
      case RAINBOW: {
          if (millis() - lastUpdate > settings.transitions.rainbowRate)
          {
            rainbowEffect(transition_step++);
            lastUpdate = millis();
          }
          break;
        }
      default: { }
    }
  }
  else
  {
    setColor(0, 0, 0, 0); //turn off all LEDs
  }

#ifdef ENABLE_MQTT
  //send current state update every xx seconds if turned on
  //update MQTT every CONFIG_MQTT_UPDATE_TIME if on; 10 * CONFIG_MQTT_UPDATE_TIME if off
  if (ENABLE_SEND_STATE && (millis() - mqtt_lastUpdate) >= (CONFIG_MQTT_UPDATE_TIME * (stateOn?1:10)) )
  {
       sendState(); //send update to MQTT
       mqtt_lastUpdate = millis();

       Serial.print(getTotalAvailableMemory());
       Serial.print(' ');
       Serial.print(getLargestAvailableBlock());
       Serial.print(' ');
       Serial.println(getFragmentation());
  }
#endif

  //send current state update every xx seconds if turned on
  if ((millis() - lastSocketUpdate) >= CONFIG_WEBSOCKET_UPDATE_TIME) //send status to any open WebSockets
  {
    sendStatusUpdate();  //send update to WebSocket clients
    lastSocketUpdate = millis();
  }

#ifdef DEBUG_MEMORY
  uint32_t freeMem = system_get_free_heap_size();
  if (lastMem != freeMem && ((stateOn && (lastMemCheck - millis() > 10000)) || (!stateOn && (lastMemCheck - millis() > 10000))))
  {
    Serial.print(F("Free Heap: "));
    Serial.println(freeMem);
    lastMem = freeMem;
    lastMemCheck = millis();
  }
#endif
}

#ifdef ENABLE_TEMPERATURE_SENSOR
void getTemp()
{
  tempSensor.requestTemperatures();

  int16_t conversionTime = tempSensor.millisToWaitForConversion(tempSensor.getResolution());
  // sleep() call can be replaced by wait() call if node need to process incoming messages (or if node is repeater)
  delay(conversionTime);
  
  temperature = ((int)(tempSensor.getTempCByIndex(0) * 10)) / 10.0;
}
#endif

void setColor(int inR, int inG, int inB, int inW)
{
  if (CONFIG_INVERT_LED_LOGIC) {
    inR = (255 - inR);
    inG = (255 - inG);
    inB = (255 - inB);
    inW = (255 - inW);
  }

  if (rgb)
  {
    analogWrite(CONFIG_PIN_RED, inR);
    analogWrite(CONFIG_PIN_GREEN, inG);
    analogWrite(CONFIG_PIN_BLUE, inB);
  }

  if (includeWhite)
  {
    analogWrite(CONFIG_PIN_WHITE, inW);
  }
}

void colorFadeEffect(uint8_t p_index)
{
  byte bright;

  if (p_index < 127) //fade up
  {
    bright = brightness / 128.0 * p_index;

    setColor(map(red, 0, 255, 0, bright), map(green, 0, 255, 0, bright), map(blue, 0, 255, 0, bright));
  }
  else //fade down
  {
    bright = brightness / 128.0 * (255 - p_index);
    setColor(map(red, 0, 255, 0, bright), map(green, 0, 255, 0, bright), map(blue, 0, 255, 0, bright));
  }
}

void colorFadeRotateEffect(uint8_t p_index)
{
  if (p_index == 0)
  {
    /*red = ESP8266TrueRandom.randomByte();
      green = ESP8266TrueRandom.randomByte();
      blue = ESP8266TrueRandom.randomByte();
    */

    byte newColor = ESP8266TrueRandom.random(9);  // 9 is the size of the color array
    red = colors[newColor][0];
    green = colors[newColor][1];
    blue = colors[newColor][2];
  }
  colorFadeEffect(p_index);
}

// https://github.com/xoseperez/my9291/blob/master/examples/esp8285/esp8285_rainbow.cpp
void rainbowEffect(uint8_t p_index)
{
  if (p_index < 85)
  {
    red = p_index * 3;
    green = 255 - p_index * 3;
    blue = 0;

    setColor(map(red, 0, 255, 0, brightness), map(green, 0, 255, 0, brightness), blue);
  }
  else if (p_index < 170)
  {
    p_index -= 85;

    red = 255 - p_index * 3;
    green = 0;
    blue = p_index * 3;

    setColor(map(red, 0, 255, 0, brightness), 0, map(blue, 0, 255, 0, brightness));
  }
  else
  {
    p_index -= 170;

    red = 0;
    green = p_index * 3;
    blue = 255 - p_index * 3;

    setColor(0, map(green, 0, 255, 0, brightness), map(blue, 0, 255, 0, brightness));
  }
}

size_t getTotalAvailableMemory()
{
  umm_info(0, 0);
  return ummHeapInfo.freeBlocks * block_size;
}

size_t getLargestAvailableBlock()
{
  umm_info(0, 0);
  return ummHeapInfo.maxFreeContiguousBlocks * block_size;
}

inline float getFragmentation()
{
  return 100 - getLargestAvailableBlock() * 100.0 / getTotalAvailableMemory();
}
