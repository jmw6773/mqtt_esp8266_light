#include "SPIFFS_Params.h"

//#define DEBUG_SPIFFS

char SPIFFS_VERSION[16] = "0.0.0";

void ICACHE_FLASH_ATTR beginSPIFFS()
{
  if (!SPIFFS.begin()) 
    if (SPIFFS.format()) 
    {
#ifdef DEBUG_SPIFFS
      Serial.println("SPIFFS Formatted");
#endif
    }

  if (SPIFFS.exists("/SPIFFS_VERSION"))
  {
    File spiffsVerFile = SPIFFS.open("/SPIFFS_VERSION", "r");
    if (spiffsVerFile)
    {
      spiffsVerFile.readBytes(SPIFFS_VERSION, 15);
      SPIFFS_VERSION[15] = '\0';
    }
  }
}

bool ICACHE_FLASH_ATTR saveSPIFFS_Settings()
{
   DynamicJsonBuffer configFileJB;
   JsonObject& settings = configFileJB.createObject();

   //Import General Settings
   JsonObject& general = settings.createNestedObject("general");
     general["hostname"]    = CONFIG_HOSTNAME;
     general["restart"] = autoRestartIntervalSeconds;
     general["updateinterval"] = autoUpdateIntervalSeconds;
     general["updateURL"] = CONFIG_UPDATE_URL;

   //Import Network Settings
   JsonObject& network = settings.createNestedObject("network");
     network["ssid"]   = CONFIG_WIFI_SSID;
     network["pass"]   = CONFIG_WIFI_PASS;
     network["apmode"] = CONFIG_WIFI_AP_MODE;

   //Import MQTT Settings
   JsonObject& mqtt = settings.createNestedObject("mqtt");
     mqtt["host"]  = CONFIG_MQTT_HOST;
     mqtt["port"]  = CONFIG_MQTT_PORT;
     mqtt["user"]  = CONFIG_MQTT_USER;
     mqtt["pass"]  = CONFIG_MQTT_PASS;
     mqtt["cID"]   = CONFIG_MQTT_CLIENT_ID;
  
     mqtt["state"] = CONFIG_MQTT_TOPIC_STATE;
     mqtt["set"]   = CONFIG_MQTT_TOPIC_SET;
     mqtt["temp"]  = CONFIG_MQTT_TOPIC_TEMP;
     mqtt["will"]  = CONFIG_MQTT_TOPIC_WILL;
   
   //Import NTP Settings
   JsonObject& json_ntp = settings.createNestedObject("ntp");
     json_ntp["host"]     = ntpserver;
     json_ntp["interval"] = ntp_interval;
     json_ntp["timezone"] = timeZone;
   
   //Import transitions Settings
   JsonObject& transitions = settings.createNestedObject("transitions");
     transitions["rainbow"]     = rainbowRate;
     transitions["fade"]        = fadeRate;
     transitions["randfade"]    = randFadeRate;
     transitions["numflashes"]  = CONFIG_NUM_FLASHS;
     transitions["flashlength"] = CONFIG_FLASH_LENGTH;

  File configFile = SPIFFS.open(PARAMS_FILE, "w");
  if (configFile) 
  {
    settings.prettyPrintTo(configFile);
#ifdef DEBUG_SPIFFS
    Serial.println("Writing parameter file to SPIFFS");
    settings.prettyPrintTo(Serial);
#endif
    configFile.close();
    return true;
  }
  return false;
}

bool ICACHE_FLASH_ATTR readSPIFFS_Settings() 
{  
  File configFile = SPIFFS.open(PARAMS_FILE, "r");
  
  if (!configFile) 
  {
#ifdef DEBUG_SPIFFS
    Serial.println("Parameter file not found in SPIFFS. Creating...");
#endif
    //Params don't exist, use default values and save params file
    saveSPIFFS_Settings();
    
    configFile = SPIFFS.open(PARAMS_FILE, "r");

    if (!configFile)
    { return false; }
  }
  
#ifdef DEBUG_SPIFFS
  Serial.println("File exists");
  Serial.println("");
#endif
  size_t size = configFile.size();
  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);
  configFile.close();
  
  DynamicJsonBuffer configFileJB;
  JsonObject& json = configFileJB.parseObject(buf.get());
  
  if (!json.success())
  {
#ifdef DEBUG_SPIFFS
    Serial.println("Failed to parse config file.");
    Serial.println("");
    return false;
#endif
  }

  parseGeneral(json["general"]);
  parseNetwork(json["network"]);
  parseMQTT(json["mqtt"]);
  parseNTP(json["ntp"]);
  parseTransitions(json["transitions"]);

#ifdef DEBUG_SPIFFS
    Serial.println("Config file read");
    json.prettyPrintTo(Serial);
    Serial.println("");
    Serial.println("");
#endif

  return true;
}

void ICACHE_FLASH_ATTR parseGeneral(JsonObject& general)
{
//Import General Settings
  if (general.containsKey("hostname"))
    strcpy(CONFIG_HOSTNAME, general["hostname"]);
  else
    strcpy(CONFIG_HOSTNAME, APPNAME);
    
  if (general.containsKey("restart"))
    autoRestartIntervalSeconds = general["restart"];

  if (general.containsKey("updateinterval"))
    autoUpdateIntervalSeconds = general["updateinterval"];
    
  if (general.containsKey("updateURL"))
     strcpy(CONFIG_UPDATE_URL, general["updateURL"]);
}

void ICACHE_FLASH_ATTR parseNetwork(JsonObject& network)
{
//Import Network Settings
  if (network.containsKey("ssid"))
    strcpy(CONFIG_WIFI_SSID, network["ssid"]);
    
  if (network.containsKey("pass"))
    strcpy(CONFIG_WIFI_PASS, network["pass"]);
    
  if (network.containsKey("apmode"))
    CONFIG_WIFI_AP_MODE = network["apmode"];
}

void ICACHE_FLASH_ATTR parseMQTT(JsonObject& mqtt)
{
//Import MQTT Settings
  if (mqtt.containsKey("host"))
    strcpy(CONFIG_MQTT_HOST, mqtt["host"]);
    
  if (mqtt.containsKey("port"))
    CONFIG_MQTT_PORT = mqtt["port"];
    
  if (mqtt.containsKey("user"))
    strcpy(CONFIG_MQTT_USER, mqtt["user"]);
    
  if (mqtt.containsKey("pass"))
    strcpy(CONFIG_MQTT_PASS, mqtt["pass"]);

  if (mqtt.containsKey("cID"))
  {
    strcpy(CONFIG_MQTT_CLIENT_ID, mqtt["cID"]);
    if (strcmp(CONFIG_MQTT_CLIENT_ID,"") == 0)
    {
      strcpy(CONFIG_MQTT_CLIENT_ID, String( ESP.getChipId(), HEX ).c_str());
    }
  }
  else
    strcpy(CONFIG_MQTT_CLIENT_ID, String( ESP.getChipId(), HEX ).c_str());

  if (mqtt.containsKey("state"))
    strcpy(CONFIG_MQTT_TOPIC_STATE, mqtt["state"]);
    
  if (mqtt.containsKey("set"))
    strcpy(CONFIG_MQTT_TOPIC_SET, mqtt["set"]);
    
  if (mqtt.containsKey("temp"))
    strcpy(CONFIG_MQTT_TOPIC_TEMP, mqtt["temp"]);
    
  if (mqtt.containsKey("will"))
    strcpy(CONFIG_MQTT_TOPIC_WILL, mqtt["will"]);
}

void ICACHE_FLASH_ATTR parseNTP(JsonObject& json_ntp)
{
//Import NTP Settings
  if (json_ntp.containsKey("host"))
    strcpy(ntpserver, json_ntp["host"]);
    
  if (json_ntp.containsKey("interval"))
    ntp_interval = json_ntp["interval"];
    
  if (json_ntp.containsKey("timezone"))
    timeZone = json_ntp["timezone"];
}

void ICACHE_FLASH_ATTR parseTransitions(JsonObject& transitions)
{
//Import transitions Settings

  if (transitions.containsKey("rainbow"))
    rainbowRate = transitions["rainbow"];
  
  if (transitions.containsKey("fade"))
    fadeRate = transitions["fade"];
  
  if (transitions.containsKey("randfade"))
    randFadeRate = transitions["randfade"];
  
  if (transitions.containsKey("flashlength"))
    CONFIG_FLASH_LENGTH = transitions["flashlength"];
    
  if (transitions.containsKey("numflashes"))
    CONFIG_NUM_FLASHS = transitions["numflashes"];
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
#ifdef DEBUG
  Serial.println("");
  Serial.println("Formatting SPIFFS");
  Serial.println("");
#endif

  SPIFFS.format();
}

// Factory Reset, removes the config.json file from SPIFFS
bool ICACHE_FLASH_ATTR removeParams()
{
#ifdef DEBUG
  Serial.println("");
  Serial.println("Deleting Param File");
  Serial.println("");
#endif

  return SPIFFS.remove(PARAMS_FILE);
}
