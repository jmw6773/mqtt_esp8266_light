#pragma once
#define APPNAME "MQTT_RGB_LED"
#define VERSION "0.1.1"
#define MODEBUTTON 0

#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
  #define DEBUG_SPIFFS
  #define DEBUG_STATE
//  #define DEBUG_MEMORY
  #define DEBUG_MQTT
#endif

#define ENABLE_MQTT
#define ENABLE_TEMPERATURE_SENSOR
#define ENABLE_NTP

#define MAX_CONFIG_SIZE 1792

#include "NTP.h"

extern double temperature;
extern bool stateOn; //lights current state
extern byte brightness; //lights brightness state

extern bool ENABLE_SEND_STATE;

//version style: 00.00.00
extern char SPIFFS_VERSION[9];

enum transitions {
  OFF=0,
  SOLID=1,
  FADE=2,
  RANDOM_FADE=3,
  FLASH=4,
  RAINBOW=5
};

// Reverse the LED logic
// false: 0 (off) - 255 (bright)
// true: 255 (off) - 0 (bright)
#define CONFIG_INVERT_LED_LOGIC false

// Set the mode for the built-in LED on some boards.
// -1 = Do nothing. Leave the pin in its default state.
//  0 = Explicitly set the BUILTIN_LED to LOW.
//  1 = Explicitly set the BUILTIN_LED to HIGH. (Off for Wemos D1 Mini)
#define CONFIG_BUILTIN_LED_MODE -1




struct GeneralSettings {
  char CONFIG_HOSTNAME[32] = "Undefined";
  unsigned int autoRestartIntervalSeconds = 86400; //seconds between forced reboots  (0 - 4,294,967,295 --- max is 49 days on ESP8266)
};

struct OTASettings {
  char CONFIG_UPDATE_URL[64] = "http://192.168.1.2:90/ota/esp8266-v1.php";
  unsigned int autoUpdateIntervalSeconds  = 1209600;  //seconds between update checks  (0 - 4,294,967,295 --- max is 49 days on ESP8266)
};

struct NetworkSettings {
//  char CONFIG_WIFI_SSID[32] = "OIST-Workshop"; //"kcender2.4";
//  char CONFIG_WIFI_PASS[32] = "workshop"; //"iloveitsumi";
  char CONFIG_WIFI_SSID[32] = "kcender2.4";
  char CONFIG_WIFI_PASS[32] = "iloveitsumi";
  bool CONFIG_WIFI_AP_MODE  = false;
};

struct MQTTSettings {
  int  CONFIG_MQTT_PORT          = 1883; // Usually 1883
  char CONFIG_MQTT_HOST[32]      = "192.168.1.2";
  char CONFIG_MQTT_USER[32]      = "couch_lights";
  char CONFIG_MQTT_PASS[32]      = "lettherebelight";
//char CONFIG_MQTT_HOST[32]        = "test.mosquitto.org";
//  char CONFIG_MQTT_USER[32]      = "";
//  char CONFIG_MQTT_PASS[32]      = "";
  char CONFIG_MQTT_CLIENT_ID[16];
  bool MQTT_USE_AUTHENTICATION   = true;

  char CONFIG_MQTT_TOPIC_STATE[32] = "devices/lights/device/rgb";
  char CONFIG_MQTT_TOPIC_SET[32]   = "devices/lights/device/set";
  char CONFIG_MQTT_TOPIC_TEMP[32]  = "devices/lights/device/temp";
  char CONFIG_MQTT_TOPIC_WILL[32]  = "devices/lights/device/will";
};

struct NTPSettings {
  char ntpserver[32] = "0.jp.pool.ntp.org";
  int timeZone       = 9; //timezone
};

struct TransisionSettings {
  byte rainbowRate   = 30;
  byte fadeRate      = 30;
  byte randFadeRate  = 30;
  
  // Default number of flashes if no value was given
  int CONFIG_FLASH_LENGTH = 500;
  byte CONFIG_NUM_FLASHS  = 3;
};

struct SETTINGS {
  GeneralSettings general;
  OTASettings ota;
  NetworkSettings network;
  MQTTSettings mqtt;
  NTPSettings ntp;
  TransisionSettings transitions;
};

extern SETTINGS settings;


// ---------- MQTT Settings ----------
#define CONFIG_MQTT_PAYLOAD_ON "ON"
#define CONFIG_MQTT_PAYLOAD_OFF "OFF"

#define CONFIG_MQTT_UPDATE_TIME 500
#define CONFIG_MQTT_TEMP_TIME 600000


// ---------- NTP Settings ----------
extern NtpClient NTP;


// --------- Transitions Settings ----------
extern transitions current_transition;
extern transitions previous_transition;
