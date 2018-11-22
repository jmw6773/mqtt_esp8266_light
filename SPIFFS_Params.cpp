#include "SPIFFS_Params.h"

char SPIFFS_VERSION[9] = "0.0.0";

void ICACHE_FLASH_ATTR beginSPIFFS()
{
  if (!SPIFFS.begin()) 
    if (SPIFFS.format()) 
    {
#ifdef DEBUG_SPIFFS
      Serial.println(F("SPIFFS Formatted"));
#endif
    }
  
  if (SPIFFS.exists(F("/SPIFFS_VERSION")))
  {
    File spiffsVerFile = SPIFFS.open(F("/SPIFFS_VERSION"), "r");
    if (spiffsVerFile)
    {
      int bytesRead = spiffsVerFile.readBytes(SPIFFS_VERSION, 15);
      SPIFFS_VERSION[bytesRead] = '\0';
      spiffsVerFile.close();
    }
  }
}

bool ICACHE_FLASH_ATTR saveSPIFFS_Settings()
{
   //DynamicJsonBuffer configFileJB(MAX_CONFIG_SIZE);
   StaticJsonBuffer<MAX_CONFIG_SIZE> configFileJB;
   JsonObject& spiffs_settings = configFileJB.createObject();

   //Import General Settings
   JsonObject& general = spiffs_settings.createNestedObject(F("general"));
     general[F("hostname")]       = settings.general.CONFIG_HOSTNAME;
     general[F("restart")]        = settings.general.autoRestartIntervalSeconds;
     
   JsonObject& otaUpdates = spiffs_settings.createNestedObject(F("ota"));
     otaUpdates[F("updateinterval")] = settings.ota.autoUpdateIntervalSeconds;
     otaUpdates[F("updateURL")]      = settings.ota.CONFIG_UPDATE_URL;

   //Import Network Settings
   JsonObject& network = spiffs_settings.createNestedObject(F("network"));
     network[F("ssid")]   = settings.network.CONFIG_WIFI_SSID;
     network[F("pass")]   = settings.network.CONFIG_WIFI_PASS;
     network[F("apmode")] = settings.network.CONFIG_WIFI_AP_MODE;

   //Import MQTT Settings
   JsonObject& mqtt = spiffs_settings.createNestedObject(F("mqtt"));
     mqtt[F("host")]  = settings.mqtt.CONFIG_MQTT_HOST;
     mqtt[F("port")]  = settings.mqtt.CONFIG_MQTT_PORT;
     mqtt[F("user")]  = settings.mqtt.CONFIG_MQTT_USER;
     mqtt[F("pass")]  = settings.mqtt.CONFIG_MQTT_PASS;
     mqtt[F("auth")]  = settings.mqtt.MQTT_USE_AUTHENTICATION;

     if(settings.mqtt.CONFIG_MQTT_CLIENT_ID == NULL || strcmp(settings.mqtt.CONFIG_MQTT_CLIENT_ID,"") == 0)
     {
       itoa(ESP.getChipId(), settings.mqtt.CONFIG_MQTT_CLIENT_ID, 16);
     }
     mqtt[F("cID")] = settings.mqtt.CONFIG_MQTT_CLIENT_ID;
  
     mqtt[F("state")] = settings.mqtt.CONFIG_MQTT_TOPIC_STATE;
     mqtt[F("set")]   = settings.mqtt.CONFIG_MQTT_TOPIC_SET;
     mqtt[F("temp")]  = settings.mqtt.CONFIG_MQTT_TOPIC_TEMP;
     mqtt[F("will")]  = settings.mqtt.CONFIG_MQTT_TOPIC_WILL;
   
   //Import NTP Settings
   JsonObject& json_ntp = spiffs_settings.createNestedObject(F("ntp"));
     json_ntp[F("host")]     = settings.ntp.ntpserver;
     json_ntp[F("timezone")] = settings.ntp.timeZone;
   
   //Import transitions Settings
   JsonObject& transitions = spiffs_settings.createNestedObject(F("transitions"));
     transitions[F("rainbow")]     = settings.transitions.rainbowRate;
     transitions[F("fade")]        = settings.transitions.fadeRate;
     transitions[F("randfade")]    = settings.transitions.randFadeRate;
     transitions[F("numflashes")]  = settings.transitions.CONFIG_NUM_FLASHS;
     transitions[F("flashlength")] = settings.transitions.CONFIG_FLASH_LENGTH;
   
   
   File configFile = SPIFFS.open(CONFIG_FILE, "w");
   if (configFile) 
   {
#ifdef DEBUG_SPIFFS
     Serial.print(F("Writing parameter file to SPIFFS.\nFile Name: "));
     Serial.println(CONFIG_FILE);
     spiffs_settings.prettyPrintTo(Serial);
    
     Serial.println(F("Begin Writting..."));
#endif

     spiffs_settings.prettyPrintTo(configFile);
     configFile.close();

////////////////////////////////////////
////////////////////////////////////////
////////////////////////////////////////
  Serial.print("saveSPIFFS_Settings JSON Size: ");
  Serial.println(configFileJB.size());
////////////////////////////////////////
////////////////////////////////////////
////////////////////////////////////////
     
     configFileJB.clear();
     return true;
   }

////////////////////////////////////////
////////////////////////////////////////
////////////////////////////////////////
  Serial.print("saveSPIFS JSON Size: ");
  Serial.println(configFileJB.size());
////////////////////////////////////////
////////////////////////////////////////
////////////////////////////////////////
  
  configFileJB.clear();
  return false;
}

bool ICACHE_FLASH_ATTR readSPIFFS_Settings() 
{  
  File configFile = SPIFFS.open(CONFIG_FILE, "r");
  
  if (!configFile) 
  {
#ifdef DEBUG_SPIFFS
    Serial.println(F("Parameter file not found in SPIFFS. Creating..."));
#endif
    //Params don't exist, use default values and save params file
    saveSPIFFS_Settings();
    
    configFile = SPIFFS.open(CONFIG_FILE, "r");

    if (!configFile)
    { return false; }
  }
  
#ifdef DEBUG_SPIFFS
  Serial.println(F("File exists\n"));
#endif
  size_t size = configFile.size();
  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);
  configFile.close();
  
  StaticJsonBuffer<MAX_CONFIG_SIZE> configFileJB;
  JsonObject& json = configFileJB.parseObject(buf.get());
  
  if (!json.success())
  {
#ifdef DEBUG_SPIFFS
    Serial.println(F("Failed to parse config file.\n"));
    return false;
#endif
  }

  parseGeneral(json[F("general")]);
  parseUpdate(json[F("ota")]);
  parseNetwork(json[F("network")]);
  parseMQTT(json[F("mqtt")]);
  parseNTP(json[F("ntp")]);
  parseTransitions(json[F("transitions")]);

#ifdef DEBUG_SPIFFS
    Serial.println(F("Config file read"));
    json.prettyPrintTo(Serial);
    Serial.println(F("\n"));
#endif

////////////////////////////////////////
////////////////////////////////////////
////////////////////////////////////////
  Serial.print("readSPIFFS_Settings JSON Size: ");
  Serial.println(configFileJB.size());
////////////////////////////////////////
////////////////////////////////////////
////////////////////////////////////////

  configFileJB.clear();
  
  return true;
}

void ICACHE_FLASH_ATTR parseGeneral(JsonObject& general)
{
  if (general.containsKey(F("hostname")))
    strcpy(settings.general.CONFIG_HOSTNAME, general[F("hostname")]);
  else
    strcpy(settings.general.CONFIG_HOSTNAME, APPNAME);
    
  if (general.containsKey(F("restart")))
    settings.general.autoRestartIntervalSeconds = general[F("restart")];
}

void ICACHE_FLASH_ATTR parseUpdate(JsonObject& updateSettings)
{
  if (updateSettings.containsKey(F("updateURL")))
     strcpy(settings.ota.CONFIG_UPDATE_URL, updateSettings[F("updateURL")]);
     
  if (updateSettings.containsKey(F("updateinterval")))
    settings.ota.autoUpdateIntervalSeconds = updateSettings[F("updateinterval")];
}

void ICACHE_FLASH_ATTR parseNetwork(JsonObject& network)
{
  if (network.containsKey(F("ssid")))
    strcpy(settings.network.CONFIG_WIFI_SSID, network[F("ssid")]);
    
  if (network.containsKey(F("pass")))
    strcpy(settings.network.CONFIG_WIFI_PASS, network[F("pass")]);
    
  if (network.containsKey(F("apmode")))
    settings.network.CONFIG_WIFI_AP_MODE = network[F("apmode")];
}

void ICACHE_FLASH_ATTR parseMQTT(JsonObject& mqtt)
{
//Import MQTT Settings
  if (mqtt.containsKey(F("host")))
    strcpy(settings.mqtt.CONFIG_MQTT_HOST, mqtt[F("host")]);
    
  if (mqtt.containsKey(F("port")))
    settings.mqtt.CONFIG_MQTT_PORT = mqtt[F("port")];
    
  if (mqtt.containsKey(F("user")))
    strcpy(settings.mqtt.CONFIG_MQTT_USER, mqtt[F("user")]);
    
  if (mqtt.containsKey(F("pass")))
    strcpy(settings.mqtt.CONFIG_MQTT_PASS, mqtt[F("pass")]);

  if (mqtt.containsKey(F("auth")))
    settings.mqtt.MQTT_USE_AUTHENTICATION = mqtt[F("auth")];


  if(settings.mqtt.CONFIG_MQTT_CLIENT_ID == NULL || strcmp(settings.mqtt.CONFIG_MQTT_CLIENT_ID,"") == 0)
    itoa(ESP.getChipId(), settings.mqtt.CONFIG_MQTT_CLIENT_ID, 16);

  if (mqtt.containsKey(F("cID")) && mqtt[F("cID")] != NULL && strcmp(mqtt[F("cID")],"") != 0)
    strcpy(settings.mqtt.CONFIG_MQTT_CLIENT_ID, mqtt[F("cID")]);

  if (mqtt.containsKey(F("state")))
    strcpy(settings.mqtt.CONFIG_MQTT_TOPIC_STATE, mqtt[F("state")]);
    
  if (mqtt.containsKey(F("set")))
    strcpy(settings.mqtt.CONFIG_MQTT_TOPIC_SET, mqtt[F("set")]);
    
  if (mqtt.containsKey(F("temp")))
    strcpy(settings.mqtt.CONFIG_MQTT_TOPIC_TEMP, mqtt[F("temp")]);
    
  if (mqtt.containsKey(F("will")))
    strcpy(settings.mqtt.CONFIG_MQTT_TOPIC_WILL, mqtt[F("will")]);
}

void ICACHE_FLASH_ATTR parseNTP(JsonObject& json_ntp)
{
//Import NTP Settings
  if (json_ntp.containsKey(F("host")))
    strcpy(settings.ntp.ntpserver, json_ntp[F("host")]);
    
  if (json_ntp.containsKey(F("timezone")))
    settings.ntp.timeZone = json_ntp[F("timezone")];
}

void ICACHE_FLASH_ATTR parseTransitions(JsonObject& transitions)
{
//Import transitions Settings

  if (transitions.containsKey(F("rainbow")))
    settings.transitions.rainbowRate = transitions[F("rainbow")];
  
  if (transitions.containsKey(F("fade")))
    settings.transitions.fadeRate = transitions[F("fade")];
  
  if (transitions.containsKey(F("randfade")))
    settings.transitions.randFadeRate = transitions[F("randfade")];
  
  if (transitions.containsKey(F("flashlength")))
    settings.transitions.CONFIG_FLASH_LENGTH = transitions[F("flashlength")];
    
  if (transitions.containsKey(F("numflashes")))
    settings.transitions.CONFIG_NUM_FLASHS = transitions[F("numflashes")];
}

// Factory Reset, formats the SPIFFS
// ********************************
// ********************************
// Remove this function and the server handle when in production
//  This will brick the device and require a reflash
// ********************************
// ********************************
void ICACHE_FLASH_ATTR format()
{
#ifdef ENABLE_DEBUG
  Serial.println(F("\nFormatting SPIFFS\n"));
#endif

  SPIFFS.format();
}

// Factory Reset, removes the config.json file from SPIFFS
bool ICACHE_FLASH_ATTR removeParams()
{
#ifdef ENABLE_DEBUG
  Serial.println(F("\nDeleting Param File\n"));
#endif

  return SPIFFS.remove(CONFIG_FILE);
}
