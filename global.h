#pragma once
#define APPNAME "MQTT_RGB_LED"
#define VERSION "0.1.1"
#define MODEBUTTON 0

#include "Ntp.h"

extern double temperature;
extern bool stateOn; //lights current state
extern byte brightness; //lights brightness state

extern bool ENABLE_SEND_STATE;

extern char SPIFFS_VERSION[16];

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


// ---------- General Settings ----------
extern unsigned int autoRestartIntervalSeconds; //seconds divided by 1000  (0 - 4,294,967,295 --- max is 49 days on ESP8266)
extern unsigned int autoUpdateIntervalSeconds;  //seconds between update checks  (0 - 4,294,967,295 --- max is 49 days on ESP8266)
extern char CONFIG_HOSTNAME[32];
extern char CONFIG_UPDATE_URL[64];


// ---------- Network Settings ----------
extern char CONFIG_WIFI_SSID[32];
extern char CONFIG_WIFI_PASS[32];
extern bool CONFIG_WIFI_AP_MODE;



// ---------- MQTT Settings ----------
extern char CONFIG_MQTT_HOST[32];
extern int  CONFIG_MQTT_PORT; // Usually 1883
extern char CONFIG_MQTT_USER[32];
extern char CONFIG_MQTT_PASS[32];
extern char CONFIG_MQTT_CLIENT_ID[16];

extern char CONFIG_MQTT_TOPIC_STATE[32];
extern char CONFIG_MQTT_TOPIC_SET[32];
extern char CONFIG_MQTT_TOPIC_TEMP[32];
extern char CONFIG_MQTT_TOPIC_WILL[32];

#define CONFIG_MQTT_PAYLOAD_ON "ON"
#define CONFIG_MQTT_PAYLOAD_OFF "OFF"

#define CONFIG_MQTT_UPDATE_TIME 500
#define CONFIG_MQTT_TEMP_TIME 600000


// ---------- NTP Settings ----------
extern NtpClient NTP;
extern char ntpserver[32];
extern int ntp_interval;
extern int timeZone;


// --------- Transitions Settings ----------
extern transitions current_transition;
extern transitions previous_transition;

extern byte rainbowRate;
extern byte fadeRate;
extern byte randFadeRate;
// Default number of flashes if no value was given
extern int CONFIG_FLASH_LENGTH;
extern byte CONFIG_NUM_FLASHS;
