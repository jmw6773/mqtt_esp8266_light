#pragma once

WiFiClient espClient;

#ifdef ENABLE_MQTT
  #include <PubSubClient.h>
  #define MQTT_MAX_PACKET_SIZE 1024
  
  PubSubClient client(espClient);
  #define MQTT_RECONNECT_INTERVAL 5000 //only try to reconnect to MQTT once every x seconds
  unsigned long lastMQTTAttempt = 0L;
#endif

#define CONFIG_WEBSOCKET_UPDATE_TIME 500
unsigned long lastSocketUpdate = 0L;

void ICACHE_FLASH_ATTR updateMyESP();
void ICACHE_FLASH_ATTR setup_wifi();
void sendState();
void ICACHE_FLASH_ATTR reconnect();
void setColor(int inR, int inG, int inB, int inW=0);
void getTemp();
void colorFadeEffect(uint8_t p_index);
void colorFadeRotateEffect(uint8_t p_index);
void rainbowEffect(uint8_t p_index);

bool stateOn;       //lights current state
bool lastState;

#ifdef ENABLE_TEMPERATURE_SENSOR
  #define TEMP_UPDATE_INTERVAL 5000 //update temp every 5 secs
  double temperature; //temperature reading
  unsigned long lastTempUpdate = 0L;
  byte sendTempStatusToMQTT = 0;
#endif

SETTINGS settings;

transitions current_transition = OFF;
transitions previous_transition = OFF;

// RGB Light Settings
// Leave this here. These are the choices for CONFIG_STRIP below.
enum strip {
  BRIGHTNESS, // only one color/only white
  RGB,        // RGB LEDs
  RGBW        // RGB LEDs with an extra white LED per LED
};
#define CONFIG_STRIP RGB // Choose one of the options from above.

byte timesCheckedUpdatesSinceReboot = 1;
bool checkForUpdate = false;
unsigned long deviceUptimeSec;
unsigned long lastButtonPress = 0L;
static unsigned long mqtt_lastUpdate = 0;  //last time update was sent to MQTT
const bool rgb = (CONFIG_STRIP == RGB) || (CONFIG_STRIP == RGBW);
const bool includeWhite = (CONFIG_STRIP == BRIGHTNESS) || (CONFIG_STRIP == RGBW);


// Maintained state for reporting to HA
byte red        = 0;
byte green      = 0;
byte blue       = 0;
byte white      = 0;
byte brightness = 255;

// Real values to write to the LEDs (ex. including brightness and state)
byte realRed   = 0;
byte realGreen = 0;
byte realBlue  = 0;
byte realWhite = 0;

// Pins
// In case of BRIGHTNESS: only WHITE is used
// In case of RGB(W): red, green, blue(, white) is used
// All values need to be present, if they are not needed, set to -1,
// it will be ignored.
#define CONFIG_PIN_RED     12  // For RGB(W)
#define CONFIG_PIN_GREEN   13  // For RGB(W)
#define CONFIG_PIN_BLUE    14  // For RGB(W)
#define CONFIG_PIN_WHITE   -1 // For BRIGHTNESS and RGBW

// {red, grn, blu, wht}
const byte colors[][4] = {
  {255, 0, 0, 0},     // red
  {0, 255, 0, 0},     // green
  {0, 0, 255, 0},     // blue
  {255, 80, 0, 0},    // orange
  {140, 0, 255, 0},   // purple
  {255, 0, 255, 0},   // fuchsia
  {0, 255, 255, 0},   // teal
  {255, 255, 0, 0},   // yellow
  {255, 255, 255, 0}  // white
};
const int numColors = 8;

//  ------------- Transitions Settings -----------------
static unsigned long lastUpdate = 0;       //last time transition was updated
static uint8_t transition_step  = 0;        //current step through the current transition

// Globals for fade/transitions
bool startFade         = false;
unsigned long lastLoop = 0;
bool inFade            = false;
int loopCount          = 0;
int stepR, stepG, stepB, stepW;
int redVal, grnVal, bluVal, whtVal;

//---------- Flash ----------
byte flashCount         = 0;
byte flashBrightness = brightness;

//-------- Standard - Non editable -------
bool flash      = false;
bool startFlash = false;
unsigned long flashStartTime = 0;
byte flashRed   = red;
byte flashGreen = green;
byte flashBlue  = blue;
byte flashWhite = white;

// Globals for colorfade
bool colorfade   = false;
int currentColor = 0;
