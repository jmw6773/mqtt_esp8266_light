#include "ServerSide.h"

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer updateServer(true);

#ifdef ENABLE_WEBSOCKET_SERVER
  WebSocketsServer webSocket(81);    // create a websocket server on port 81
#endif //ENABLE_WEBSOCKET_SERVER

bool inAPMode = false;

void ICACHE_FLASH_ATTR beginServerServices()
{
  startServer();
  
#ifdef ENABLE_WEBSOCKET_SERVER
  startWebSocket();
#endif //ENABLE_WEBSOCKET_SERVER
}

void ICACHE_FLASH_ATTR connectNetwork()
{
#ifdef SERVER_DEBUG
  Serial.println("Connecting WiFi Network");
#endif
  if (CONFIG_WIFI_AP_MODE)
    connectWIFI_AP_Mode();
  else
    if (!connectWIFI_STA_Mode())
    {
      if (!connectWIFI_AP_Mode())
        fallbacktoAPMode();
    }
}

bool ICACHE_FLASH_ATTR connectWIFI_STA_Mode()
{
  WiFi.mode(WIFI_STA);
  // First connect to a wi-fi network
  WiFi.begin(CONFIG_WIFI_SSID, CONFIG_WIFI_PASS);
  
#ifdef SERVER_DEBUG
  // Inform user we are trying to connect
  Serial.print(F("[ INFO ] Trying to connect WiFi: "));
  Serial.println(CONFIG_WIFI_SSID);
#endif 

  // We try it for 20 seconds and give up on if we can't connect
  unsigned long now = millis();
  uint8_t timeout = 20; // define when to time out in seconds
  // Wait until we connect or 20 seconds pass
  do {
      if (WiFi.status() == WL_CONNECTED)
      {
          break;
      }
      delay(500);
      
#ifdef SERVER_DEBUG
      Serial.print(F("."));
#endif

  }
  while (millis() - now < timeout * 1000);
  // We now out of the while loop, either time is out or we connected. check what happened
  if (WiFi.status() == WL_CONNECTED)
  { // Assume time is out first and check
      inAPMode = false;
#ifdef SERVER_DEBUG
      Serial.println();
      Serial.print(F("[ INFO ] Client IP address: ")); // Great, we connected, inform
      Serial.println(WiFi.localIP());

      Serial.print(F("[ INFO ]  WiFi is connected  -  "));
      Serial.print(CONFIG_WIFI_SSID);
      Serial.println("");
#endif
      return true;
  }
  else
  { // We couln't connect, time is out, inform
    
#ifdef SERVER_DEBUG
      Serial.println();
      Serial.println(F("[ WARN ] Couldn't connect in time"));
#endif

      return false;
  }
}

bool ICACHE_FLASH_ATTR connectWIFI_AP_Mode()
{
  // check if CONFIG_WIFI_SSID is empty
  if (strcmp(CONFIG_WIFI_SSID, "") == 0)
  { strcpy(CONFIG_WIFI_SSID, APPNAME); }
  
  inAPMode = true;
  WiFi.mode(WIFI_AP);
  
#ifdef SERVER_DEBUG
  Serial.print(F("[ INFO ] Configuring access point... "));
#endif

  bool success = WiFi.softAP(CONFIG_WIFI_SSID, CONFIG_WIFI_PASS);

#ifdef SERVER_DEBUG
  Serial.println(success ? F("WiFi AP Ready") : F("WiFi AP Failed!"));
#endif

  if (!success)
    ESP.restart();
  
#ifdef SERVER_DEBUG
  // Access Point IP
  Serial.print(F("[ INFO ] AP IP address: "));
  Serial.println(WiFi.softAPIP());
  Serial.printf("[ INFO ] AP SSID: %s\n", CONFIG_WIFI_SSID);
#endif

  return success;
}

// Fallback to AP Mode, so we can connect to ESP if there is no Internet connection
void ICACHE_FLASH_ATTR fallbacktoAPMode()
{
    inAPMode = true;
#ifdef SERVER_DEBUG
    Serial.println(F("[ INFO ] ESP is running in Fallback AP Mode"));
#endif

    uint8_t macAddr[6];
    WiFi.softAPmacAddress(macAddr);
    char ssid[15];
    sprintf(ssid, "RGB-Lights-%02x%02x%02x", macAddr[3], macAddr[4], macAddr[5]);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid);
}

void ICACHE_FLASH_ATTR startServer()
{ 
#ifdef DIRECT_UPDATE
  updateServer.setup(&server);
#endif

  server.serveStatic("/", SPIFFS, "/", "max-age=86400");
  
  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
  
  if ( MDNS.begin ( CONFIG_HOSTNAME ) ) {
#ifdef SERVER_DEBUG
    Serial.println ( "MDNS responder started" );
#endif
  }

  server.begin();                             // start the HTTP server
#ifdef SERVER_DEBUG
  Serial.println("HTTP server started.");
#endif
}

#ifdef ENABLE_WEBSOCKET_SERVER
void ICACHE_FLASH_ATTR startWebSocket()
{
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
#ifdef SERVER_DEBUG
  Serial.println("WebSocket server started.");
#endif
}

// When a WebSocket message is received
void ICACHE_FLASH_ATTR webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t payload_length)
{ 
  switch (type) 
  {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
    {
#ifdef SERVER_DEBUG
        Serial.println("Client Disconnected");
#endif
      break;
    }
    case WStype_CONNECTED:               // if a new websocket connection is established
    {
        int numClients = webSocket.connectedClients(false);
        for (int clientLoopNum=0; clientLoopNum < numClients; clientLoopNum++)
        {
          
          if(clientLoopNum != num && webSocket.remoteIP(num) == webSocket.remoteIP(clientLoopNum)) //new socket from same client, close old socket
          {
#ifdef SERVER_DEBUG
            Serial.println("New connection from same IP found. Closing old connection.");
            Serial.print("Client IP: ");
            Serial.println(webSocket.remoteIP(clientLoopNum).toString());
#endif
            webSocket.disconnect(clientLoopNum);
          }
        }
        
#ifdef SERVER_DEBUG
        Serial.println("Client Connected");
#endif
        break;
    }
    case WStype_TEXT:                     // if new text data is received
    {
      if(payload_length == 0)
      {
        return;
      }

      DynamicJsonBuffer configFileJB;
      JsonObject& json = configFileJB.parseObject(payload);
      
#ifdef SERVER_DEBUG
      json.prettyPrintTo(Serial);
#endif

      DynamicJsonBuffer response;
      JsonObject& responseJSON = response.createObject();
      
      if (json.containsKey(F("command")))
      {  
         switch(((const char*)json[F("command")])[0])
         {
           case 'c': { //save/commit all changes to SPIFFS
                       responseJSON[F("result")] = saveSPIFFS_Settings();
                       responseJSON[F("command")] = "c";
                       break;
                     }
           case 'd': { //discard all changes (reload config from SPIFFS
                       responseJSON[F("result")] = readSPIFFS_Settings();
                       responseJSON[F("command")] = "d";
                       break;
                     }
           case 'u': { //check for updates
                       //this has no JSON response as the function will reboot if there is an update
                       responseJSON[F("result")] = checkForUpdates();
                       responseJSON[F("command")] = "u";
                       break;
                     }
           case 'r': { //reboot ESP
                       responseJSON[F("result")] = true;
                       responseJSON[F("command")] = "r";
                       
                       String jsonStringBuffer;
                       responseJSON.printTo(jsonStringBuffer);
                       webSocket.broadcastTXT(jsonStringBuffer);
                       
                       delay(100);
                       restartESP();
                       break;
                     }
           case 'f': { //factory reset
                       responseJSON[F("result")] = removeParams();
                       responseJSON[F("command")] = "f";
                       
                       String jsonStringBuffer;
                       responseJSON.printTo(jsonStringBuffer);
                       webSocket.broadcastTXT(jsonStringBuffer); 
                       
                       delay(100);
                       restartESP();
                       break;
                     }
           case 'p': { //Count number of connected clients
                       int result = webSocket.connectedClients(true);
                       responseJSON[F("result")] = result;
                       responseJSON[F("command")] = "p";
#ifdef SERVER_DEBUG
                       Serial.print("\nNumber of clients: ");
                       Serial.println(result);
#endif
                       break;
                     }
           case 'z': { //format SPIFFS
                       format();
                       
                       delay(100);
                       restartESP();
                       break;
                     }
           case 's': { 
                       ENABLE_SEND_STATE = !ENABLE_SEND_STATE;
                       break;
                     }
           case 't': { 
                       stateOn = true;
                       current_transition = RAINBOW;
                       brightness = 15;
                       
                       break;
                     }
           case 'o': { 
                       stateOn = false;
                       
                       break;
                     }
         }
      }
      else if (json.containsKey(F("general")))
      {  
         responseJSON[F("command")] = F("general");
         parseGeneral(json[F("general")]);
      }
      else if (json.containsKey(F("network")))
      { 
         responseJSON[F("command")] = F("network");
          parseNetwork(json[F("network")]);
      }
      else if (json.containsKey(F("mqtt")))
      {  
         responseJSON[F("command")] = F("mqtt");
         parseMQTT(json[F("mqtt")]);
      }
      else if (json.containsKey(F("ntp")))
      {  
         responseJSON[F("command")] = F("ntp");
         parseNTP(json[F("ntp")]);
      }
      else if (json.containsKey(F("transitions")))
      {  
         responseJSON[F("command")] = F("transitions");
         parseTransitions(json[F("transitions")]);
      }
      
      String jsonStringBuffer;
      responseJSON.printTo(jsonStringBuffer);
      webSocket.broadcastTXT(jsonStringBuffer); 
      
      break;
    }
  }
}
#endif

void sendStatusUpdate()
{
#ifdef ENABLE_WEBSOCKET_SERVER
  if (webSocket.connectedClients() > 0)
  {
    struct ip_info info;
    FSInfo fsinfo;

    DynamicJsonBuffer jsonBuffer(1024);
    JsonObject& settings = jsonBuffer.createObject();

    JsonObject& device = settings.createNestedObject("device");
      device["status"] = stateOn ? "On" : "Off";
      device["version"] = VERSION;
      device["sversion"] = SPIFFS_VERSION;
#ifdef ENABLE_TEMPERATURE_SENSOR
      device["temp"] = temperature;
#else
      device["temp"] = "NA";
#endif
      device["heap"] = ESP.getFreeHeap();
      device["chip"] = String(ESP.getChipId(), HEX);
      device["cpu"] = ESP.getCpuFreqMHz();
      device["flash"] = ESP.getFreeSketchSpace();
      device["flashsize"] = ESP.getFlashChipRealSize();
      device["uptime"] = NTP.getDeviceUptimeString();

    if (SPIFFS.info(fsinfo))
    {
      device["spiffs"] = fsinfo.totalBytes - fsinfo.usedBytes;
      device["sspiffsize"] = fsinfo.totalBytes;
    }
    else
    {
       device["spiffs"] = 0;
       device["sspiffsize"] = 0;
#ifdef SERVER_DEBUG
       Serial.print(F("[ WARN ] Error getting info on SPIFFS"));
#endif
    }

    if (inAPMode) //inAPMode
    {
      wifi_get_ip_info(SOFTAP_IF, &info);
      struct softap_config conf;
      wifi_softap_get_config(&conf);
      device["ssid"] = String(reinterpret_cast<char*>(conf.ssid));
      device["mac"] = WiFi.softAPmacAddress();
    }
    else
    {
      wifi_get_ip_info(STATION_IF, &info);
      struct station_config conf;
      wifi_station_get_config(&conf);
      device["ssid"] = String(reinterpret_cast<char*>(conf.ssid));
      device["mac"] = WiFi.macAddress();
    }
    
    IPAddress ipAddr = IPAddress(info.ip.addr);
    device["ip"] = ipAddr.toString();
    
    switch (current_transition)
    {
      case FADE: {
          device["effect"] = "Fade";
          break;
        }
      case RANDOM_FADE: {
          device["effect"] = "Random Fade";
          break;
        }
      case FLASH: {
          device["effect"] = "Flash";
          break;
        }
    case RAINBOW: {
        device["effect"] = "Rainbow";
        break;
      }
    default: {
        device["effect"] = "None";
      }
    }
    
    JsonObject& general = settings.createNestedObject("general");
      general["hostname"]       = CONFIG_HOSTNAME;
      general["restart"]        = autoRestartIntervalSeconds;
      general["updateinterval"] = autoUpdateIntervalSeconds;
      general["updateURL"]      = CONFIG_UPDATE_URL;
     
    JsonObject& network = settings.createNestedObject("network");
      network["ssid"]   = CONFIG_WIFI_SSID;
      network["pass"]   = CONFIG_WIFI_PASS;
      network["apmode"] = CONFIG_WIFI_AP_MODE;

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

    JsonObject& json_ntp = settings.createNestedObject("ntp");
      json_ntp["host"] = ntpserver;
      json_ntp["update"] = ntp_interval;
      json_ntp["tz"] = timeZone;
      json_ntp["time"] = now();//NTP.getUtcTimeNow();

    JsonObject& transitions = settings.createNestedObject("transitions");
      transitions["rainbow"]     = rainbowRate;
      transitions["fade"]        = fadeRate;
      transitions["randfade"]    = randFadeRate;
      transitions["numflashes"]  = CONFIG_NUM_FLASHS;
      transitions["flashlength"] = CONFIG_FLASH_LENGTH;

    String jsonStringBuffer;
    settings.printTo(jsonStringBuffer);
    webSocket.broadcastTXT(jsonStringBuffer);
  }
#endif //ENABLE_WEBSOCKET_SERVER
}

void ICACHE_FLASH_ATTR handleNotFound()
{
  server.send(404, F("text/plain"), F("404: File Not Found"));
}

void ICACHE_FLASH_ATTR restartESP()
{
#ifdef SERVER_DEBUG
  Serial.println("Rebooting ESP...");
  Serial.println("");
#endif //SERVER_DEBUG

  server.stop();
  delay(100);
  ESP.restart();
}

bool ICACHE_FLASH_ATTR checkForUpdates()
{
  bool errorOccured = false;
  
  //make sure there's an Update URL
  if (strcmp(CONFIG_UPDATE_URL, "") != 0)
  {
    bool shouldRestart = false;
    char retVal;
    
#ifdef SERVER_DEBUG
    Serial.println("Checking for Firmare Updates...");
#endif
    retVal = ESPUpdate.iotUpdaterSPIFFS(CONFIG_UPDATE_URL, SPIFFS_VERSION);
    switch(retVal)
    {
      case 'A': { break; }
      case 'U': { shouldRestart = true; saveSPIFFS_Settings(); break; }
      case 'F': { errorOccured=true; break; }
    }

#ifdef SERVER_DEBUG
    switch(retVal)
    {
      case 'A': { Serial.println("No SPIFFS updates available"); break; }
      case 'U': { Serial.println("SPIFFS Updated"); break; }
      case 'F': { Serial.println("SPIFFS Update Failure..."); break; }
    }
#endif


#ifdef SERVER_DEBUG
    Serial.println("Checking for SPIFFS Update...");
#endif
    retVal = ESPUpdate.iotUpdaterSketch(CONFIG_UPDATE_URL, VERSION);
    switch(retVal)
    {
      case 'A': { break; }
      case 'U': { shouldRestart = true; break; }
      case 'F': { errorOccured=true; break; }
    }

#ifdef SERVER_DEBUG
    switch(retVal)
    {
      case 'A': { Serial.println("No sketch updates available"); break; }
      case 'U': { Serial.println("Sketch Updated"); break; }
      case 'F': { Serial.println("Sketch Update Failure..."); break; }
    }
#endif
    
    if (shouldRestart)
    {
#ifdef ENABLE_WEBSOCKET_SERVER
       webSocket.broadcastTXT("{'command':'u','result':true}");
       webSocket.disconnect();
#endif //ENABLE_WEBSOCKET_SERVER

       restartESP();
    }
  }
  else
  { // no update URL
    errorOccured = true;
  }
  
  return errorOccured;
}
