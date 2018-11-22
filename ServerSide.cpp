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
  Serial.println(F("Connecting WiFi Network"));
#endif
  if (settings.network.CONFIG_WIFI_AP_MODE)
    connectWIFI_AP_Mode();
  else if (!connectWIFI_STA_Mode())
  {
    if (!connectWIFI_AP_Mode())
      fallbacktoAPMode();
  }
}

bool ICACHE_FLASH_ATTR connectWIFI_STA_Mode()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(settings.network.CONFIG_WIFI_SSID, settings.network.CONFIG_WIFI_PASS);
  
#ifdef SERVER_DEBUG
  Serial.print(F("[ INFO ] Trying to connect WiFi: "));
  Serial.println(settings.network.CONFIG_WIFI_SSID);
#endif

  unsigned long nowTime = millis();
  int16_t timeout = 20 * 1000; // time out in seconds
  
  do {
    if (WiFi.status() == WL_CONNECTED)
    {
      break;
    }
    delay(500);

#ifdef SERVER_DEBUG
    Serial.print(".");
#endif
  
  } while (millis() - nowTime < timeout);

  if (WiFi.status() == WL_CONNECTED)
  {
    inAPMode = false;
#ifdef SERVER_DEBUG
    Serial.print(F("\n[ INFO ] Client IP address: ")); // Great, we connected, inform
    Serial.println(WiFi.localIP());

    Serial.print(F("[ INFO ]  WiFi is connected  -  "));
    Serial.print(settings.network.CONFIG_WIFI_SSID);
    Serial.println(F(""));
#endif
    return true;
  }
  else
  {
#ifdef SERVER_DEBUG
    Serial.println(F("\n[ WARN ] Couldn't connect in time"));
#endif
    return false;
  }
}

bool ICACHE_FLASH_ATTR connectWIFI_AP_Mode()
{
  // check if CONFIG_WIFI_SSID is empty
  if (strcmp(settings.network.CONFIG_WIFI_SSID, "") == 0)
  {
    strcpy(settings.network.CONFIG_WIFI_SSID, APPNAME);
  }

  inAPMode = true;
  WiFi.mode(WIFI_AP);

#ifdef SERVER_DEBUG
  Serial.print(F("[ INFO ] Configuring access point... "));
#endif

  bool success = WiFi.softAP(settings.network.CONFIG_WIFI_SSID, settings.network.CONFIG_WIFI_PASS);

#ifdef SERVER_DEBUG
  Serial.println(success ? F("WiFi AP Ready") : F("WiFi AP Failed!"));
#endif

  if (!success)
    ESP.restart();

#ifdef SERVER_DEBUG
  // Access Point IP
  Serial.print(F("[ INFO ] AP IP address: "));
  Serial.println(WiFi.softAPIP());
  Serial.print(F("[ INFO ] AP SSID: "));
  Serial.println(settings.network.CONFIG_WIFI_SSID);
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

  if ( MDNS.begin ( settings.general.CONFIG_HOSTNAME ) ) {
#ifdef SERVER_DEBUG
    Serial.println ( F("MDNS responder started" ));
#endif
  }

  server.begin();                             // start the HTTP server
#ifdef SERVER_DEBUG
  Serial.println(F("HTTP server started."));
#endif
}

#ifdef ENABLE_WEBSOCKET_SERVER
void ICACHE_FLASH_ATTR startWebSocket()
{
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  
#ifdef SERVER_DEBUG
  Serial.println(F("WebSocket server started."));
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
        Serial.println(F("Client Disconnected"));
        Serial.println((char*)payload);
#endif
        break;
      }
    case WStype_CONNECTED:               // if a new websocket connection is established
      {
        int numClients = webSocket.connectedClients(false);
        for (int clientLoopNum = 0; clientLoopNum < numClients; clientLoopNum++)
        {
          if (clientLoopNum != num && webSocket.remoteIP(num) == webSocket.remoteIP(clientLoopNum)) //new socket from same client, close old socket
          {
#ifdef SERVER_DEBUG
            Serial.println(F("New connection from same IP found. Closing old connection."));
            Serial.print(F("Client IP: "));
            Serial.println(webSocket.remoteIP(clientLoopNum).toString());
#endif
            webSocket.disconnect(clientLoopNum);
          }
        }
        
#ifdef SERVER_DEBUG
        Serial.println(F("Client Connected"));
#endif
        sendDeviceInformation();
        break;
      }
    case WStype_TEXT:                     // if new text data is received
      {
        if (payload_length == 0)
        {
          return;
        }
        
        StaticJsonBuffer<256> configFileJB;
        JsonObject& clientCommandPacket = configFileJB.parseObject(payload);
        
#ifdef SERVER_DEBUG
        clientCommandPacket.prettyPrintTo(Serial);
        Serial.println();
#endif
        
        char responseBuffer[64];
        StaticJsonBuffer<128> response;
        JsonObject& responseJSON = response.createObject();
        
        if (clientCommandPacket.containsKey(F("command")))
        {
          switch (((const char*)clientCommandPacket[F("command")])[0])
          {
            case 'c': { //save/commit all changes to SPIFFS
                responseJSON[F("result")] = saveSPIFFS_Settings();
                responseJSON[F("command")] = F("c");
                
                send_Device_Information(); //SPIFFS changed, resend new space amount
                break;
              }
            case 'd': { //discard all changes (reload config from SPIFFS
                responseJSON[F("result")] = readSPIFFS_Settings();
                responseJSON[F("command")] = F("d");
                break;
              }
            case 'f': { //factory reset
                responseJSON[F("result")] = removeParams();
                responseJSON[F("command")] = F("f");

                responseJSON.printTo(responseBuffer, sizeof(responseBuffer));
                webSocket.broadcastTXT(responseBuffer);

                delay(100);
                restartESP();
                break;
              }
            case 'o': {
                stateOn = false;

                break;
              }
            case 'p': { //Count number of connected clients
                int result = webSocket.connectedClients(true);
                responseJSON[F("result")] = result;
                responseJSON[F("command")] = F("p");
#ifdef SERVER_DEBUG
                Serial.print(F("\nNumber of clients: "));
                Serial.println(result);
#endif
                break;
              }
            case 'r': { //reboot ESP
                webSocket.broadcastTXT("{\"result\": true,\"command\": \"r\"}");

                delay(100);
                restartESP();
                break;
              }
#ifdef ENABLE_MQTT
            case 's': {
                ENABLE_SEND_STATE = !ENABLE_SEND_STATE;
                break;
              }
#endif //ENABLE_MQTT
            case 't': { //resend the NTP device time
                responseJSON[F("result")] = NTP.getUtcTimeNow();
                responseJSON[F("command")] = F("t");
                // send_NTP_Information(); //resend all NTP information
                break;
              }
            case 'u': { //check for updates
                //this may have no JSON response as the function will reboot if there is an update
                responseJSON[F("result")] = checkForUpdates();
                responseJSON[F("command")] = F("u");
                break;
              }
///////////////////////////
//  Used for Testing
///////////////////////////
            case 'y': {
                stateOn = true;
                current_transition = RAINBOW;
                brightness = 15;

                break;
              }
//////////////////////////
            case 'z': { //format SPIFFS
                format();
                webSocket.broadcastTXT("{\"result\": true,\"command\": \"z\"}");

                delay(100);
                restartESP();
                break;
              }
          }
        }
        else if (clientCommandPacket.containsKey(F("general")))
        {
          responseJSON[F("command")] = F("general");
          parseGeneral(clientCommandPacket[F("general")]);
        }
        else if (clientCommandPacket.containsKey(F("ota")))
        {
          responseJSON[F("command")] = F("ota");
          parseUpdate(clientCommandPacket[F("ota")]);
        }
        else if (clientCommandPacket.containsKey(F("network")))
        {
          responseJSON[F("command")] = F("network");
          parseNetwork(clientCommandPacket[F("network")]);
        }
        else if (clientCommandPacket.containsKey(F("mqtt")))
        {
          responseJSON[F("command")] = F("mqtt");
          parseMQTT(clientCommandPacket[F("mqtt")]);
        }
        else if (clientCommandPacket.containsKey(F("ntp")))
        {
          responseJSON[F("command")] = F("ntp");
          parseNTP(clientCommandPacket[F("ntp")]);
        }
        else if (clientCommandPacket.containsKey(F("transitions")))
        {
          responseJSON[F("command")] = F("transitions");
          parseTransitions(clientCommandPacket[F("transitions")]);
        }
        
        responseJSON.printTo(responseBuffer, sizeof(responseBuffer));
        webSocket.broadcastTXT(responseBuffer);
        
////////////////////////////////////////
////////////////////////////////////////
////////////////////////////////////////
  Serial.print("webSocketEvent JSON Size: ");
  Serial.println(response.size());
////////////////////////////////////////
////////////////////////////////////////
////////////////////////////////////////
        
        break;
      }
  }
}
#endif

void send_Device_Information()
{
  struct ip_info info;
  FSInfo fsinfo;

  StaticJsonBuffer<MAX_CONFIG_SIZE> jsonBuffer;
  JsonObject& jsonSettings = jsonBuffer.createObject();

  JsonObject& device    = jsonSettings.createNestedObject(F("device"));
  device[F("version")]  = VERSION;
  device[F("sversion")] = SPIFFS_VERSION;

  char chipID[16];
  itoa(ESP.getChipId(), chipID, 16);

  device[F("chip")]      = chipID;
  device[F("cpu")]       = ESP.getCpuFreqMHz();
  device[F("flash")]     = ESP.getFreeSketchSpace();
  device[F("flashsize")] = ESP.getFlashChipRealSize();

  if (SPIFFS.info(fsinfo))
  {
    device[F("spiffs")]     = fsinfo.totalBytes - fsinfo.usedBytes;
    device[F("sspiffsize")] = fsinfo.totalBytes;
  }
  else
  {
    device[F("spiffs")]     = 0;
    device[F("sspiffsize")] = 0;
#ifdef SERVER_DEBUG
    Serial.print(F("[ WARN ] Error getting info on SPIFFS"));
#endif
  }
    
  if (inAPMode) //inAPMode
  {
    wifi_get_ip_info(SOFTAP_IF, &info);
    struct softap_config conf;
    wifi_softap_get_config(&conf);
    device[F("ssid")] = reinterpret_cast<char*>(conf.ssid);
    device[F("mac")]  = WiFi.softAPmacAddress();
  }
  else
  {
    wifi_get_ip_info(STATION_IF, &info);
    struct station_config conf;
    wifi_station_get_config(&conf);
    device[F("ssid")] = reinterpret_cast<char*>(conf.ssid);
    device[F("mac")]  = WiFi.macAddress();
  }
    
  IPAddress ipAddr = IPAddress(info.ip.addr);
  device[F("ip")]  = ipAddr.toString();
  
  char responseBuffer[jsonSettings.measureLength()+1];
  jsonSettings.printTo(responseBuffer, sizeof(responseBuffer));
  
  webSocket.broadcastTXT(responseBuffer);
  
////////////////////////////////////////
////////////////////////////////////////
////////////////////////////////////////
  Serial.print("send_Device_Information JSON Size: ");
  Serial.println(jsonBuffer.size());
////////////////////////////////////////
////////////////////////////////////////
////////////////////////////////////////
     
}

void send_General_Information()
{
  StaticJsonBuffer<256> jsonBuffer;
  JsonObject& jsonSettings = jsonBuffer.createObject();
  
  JsonObject& general = jsonSettings.createNestedObject(F("general"));
    general[F("hostname")]       = settings.general.CONFIG_HOSTNAME;
    general[F("restart")]        = settings.general.autoRestartIntervalSeconds;
  
  char responseBuffer[jsonSettings.measureLength()+1];
  jsonSettings.printTo(responseBuffer, sizeof(responseBuffer));
  
  webSocket.broadcastTXT(responseBuffer);
}

void send_OTA_Information()
{
  StaticJsonBuffer<256> jsonBuffer;
  JsonObject& jsonSettings = jsonBuffer.createObject();
  
  JsonObject& ota = jsonSettings.createNestedObject(F("ota"));
    ota[F("updateinterval")] = settings.ota.autoUpdateIntervalSeconds;
    ota[F("updateURL")]      = settings.ota.CONFIG_UPDATE_URL;
  
  char responseBuffer[jsonSettings.measureLength()+1];
  jsonSettings.printTo(responseBuffer, sizeof(responseBuffer));
  
  webSocket.broadcastTXT(responseBuffer);
}

void send_Network_Information()
{
  StaticJsonBuffer<256> jsonBuffer;
  JsonObject& jsonSettings = jsonBuffer.createObject();
  
  JsonObject& network = jsonSettings.createNestedObject(F("network"));
    network[F("ssid")]   = settings.network.CONFIG_WIFI_SSID;
    network[F("pass")]   = settings.network.CONFIG_WIFI_PASS;
    network[F("apmode")] = settings.network.CONFIG_WIFI_AP_MODE;
  
  char responseBuffer[jsonSettings.measureLength()+1];
  jsonSettings.printTo(responseBuffer, sizeof(responseBuffer));
  
  webSocket.broadcastTXT(responseBuffer);
}

void send_MQTT_Information()
{
  StaticJsonBuffer<512> jsonBuffer;
  JsonObject& jsonSettings = jsonBuffer.createObject();

  JsonObject& mqtt = jsonSettings.createNestedObject(F("mqtt"));
    mqtt[F("host")]  = settings.mqtt.CONFIG_MQTT_HOST;
    mqtt[F("port")]  = settings.mqtt.CONFIG_MQTT_PORT;
    mqtt[F("user")]  = settings.mqtt.CONFIG_MQTT_USER;
    mqtt[F("pass")]  = settings.mqtt.CONFIG_MQTT_PASS;
    mqtt[F("auth")]  = settings.mqtt.MQTT_USE_AUTHENTICATION;
    mqtt[F("cID")]   = settings.mqtt.CONFIG_MQTT_CLIENT_ID;
    mqtt[F("state")] = settings.mqtt.CONFIG_MQTT_TOPIC_STATE;
    mqtt[F("set")]   = settings.mqtt.CONFIG_MQTT_TOPIC_SET;
    mqtt[F("temp")]  = settings.mqtt.CONFIG_MQTT_TOPIC_TEMP;
    mqtt[F("will")]  = settings.mqtt.CONFIG_MQTT_TOPIC_WILL;
  
  char responseBuffer[jsonSettings.measureLength()+1];
  jsonSettings.printTo(responseBuffer, sizeof(responseBuffer));
  
  webSocket.broadcastTXT(responseBuffer);
}

void send_NTP_Information()
{
  StaticJsonBuffer<256> jsonBuffer;
  JsonObject& jsonSettings = jsonBuffer.createObject();

  JsonObject& json_ntp = jsonSettings.createNestedObject(F("ntp"));
    json_ntp[F("host")]   = settings.ntp.ntpserver;
    json_ntp[F("time")]   = NTP.getUtcTimeNow();
    json_ntp[F("tz")]     = settings.ntp.timeZone;
  
  char responseBuffer[jsonSettings.measureLength()+1];
  jsonSettings.printTo(responseBuffer, sizeof(responseBuffer));
  
  webSocket.broadcastTXT(responseBuffer);
}

void send_Transitions_Information()
{
  StaticJsonBuffer<256> jsonBuffer;
  JsonObject& jsonSettings = jsonBuffer.createObject();

  JsonObject& transitions = jsonSettings.createNestedObject(F("transitions"));
    transitions[F("rainbow")]     = settings.transitions.rainbowRate;
    transitions[F("fade")]        = settings.transitions.fadeRate;
    transitions[F("randfade")]    = settings.transitions.randFadeRate;
    transitions[F("numflashes")]  = settings.transitions.CONFIG_NUM_FLASHS;
    transitions[F("flashlength")] = settings.transitions.CONFIG_FLASH_LENGTH;
  
  char responseBuffer[jsonSettings.measureLength()+1];
  jsonSettings.printTo(responseBuffer, sizeof(responseBuffer));
  
  webSocket.broadcastTXT(responseBuffer);
}


//send device information on connect
void sendDeviceInformation()
{
#ifdef ENABLE_WEBSOCKET_SERVER
  if (webSocket.connectedClients() > 0)
  {
    send_Device_Information();
    send_General_Information();
    send_OTA_Information();
    send_Network_Information();
    send_MQTT_Information();
    send_NTP_Information();
    send_Transitions_Information();
  }
#endif //ENABLE_WEBSOCKET_SERVER
}

//send items that change
void sendStatusUpdate()
{
#ifdef ENABLE_WEBSOCKET_SERVER
  if (webSocket.connectedClients() > 0)
  {
    StaticJsonBuffer<384> jsonBuffer;
    JsonObject& jsonSettings = jsonBuffer.createObject();

    JsonObject& device = jsonSettings.createNestedObject(F("device"));
    device[F("status")] = stateOn ? F("On") : F("Off");
#ifdef ENABLE_TEMPERATURE_SENSOR
    device[F("temp")] = temperature;
#else
    device[F("temp")] = F("NA");
#endif
    device[F("heap")] = ESP.getFreeHeap();
    device[F("uptime")] = NTP.getUptimeSec();

    switch (current_transition)
    {
      case FADE: {
          device[F("effect")] = F("Fade");
          break;
        }
      case RANDOM_FADE: {
          device[F("effect")] = F("Random Fade");
          break;
        }
      case FLASH: {
          device[F("effect")] = F("Flash");
          break;
        }
      case RAINBOW: {
          device[F("effect")] = F("Rainbow");
          break;
        }
      default: {
          device[F("effect")] = F("None");
        }
    }
        
    char responseBuffer[jsonSettings.measureLength()+1];
    jsonSettings.printTo(responseBuffer, sizeof(responseBuffer));
    
    webSocket.broadcastTXT(responseBuffer);
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
  Serial.println(F("Rebooting ESP...\n"));
#endif //SERVER_DEBUG

  server.stop();
  delay(100);
  ESP.restart();
}

bool ICACHE_FLASH_ATTR checkForUpdates()
{
  bool errorOccured = false;

  //make sure there's an Update URL
  if (strcmp(settings.ota.CONFIG_UPDATE_URL, "") != 0)
  {
    bool shouldRestart = false;
    char retVal;

#ifdef SERVER_DEBUG
    Serial.println(F("\n\nChecking for Firmare Updates..."));
#endif
    retVal = ESPUpdate.iotUpdaterSPIFFS(settings.ota.CONFIG_UPDATE_URL, SPIFFS_VERSION);
    switch (retVal)
    {
      case 'A': {
          break;
        }
      case 'U': {
          shouldRestart = true;
          saveSPIFFS_Settings();
          break;
        }
      case 'F': {
          errorOccured = true;
          break;
        }
    }

#ifdef SERVER_DEBUG
    switch (retVal)
    {
      case 'A': {
          Serial.println(F("No SPIFFS updates available"));
          break;
        }
      case 'U': {
          Serial.println(F("SPIFFS Updated"));
          break;
        }
      case 'F': {
          Serial.println(F("SPIFFS Update Failure..."));
          break;
        }
    }
#endif


#ifdef SERVER_DEBUG
    Serial.println(F("Checking for Sketch Update..."));
#endif
    retVal = ESPUpdate.iotUpdaterSketch(settings.ota.CONFIG_UPDATE_URL, VERSION);
    switch (retVal)
    {
      case 'A': {
          break;
        }
      case 'U': {
          shouldRestart = true;
          break;
        }
      case 'F': {
          errorOccured = true;
          break;
        }
    }

#ifdef SERVER_DEBUG
    switch (retVal)
    {
      case 'A': {
          Serial.println(F("No sketch updates available"));
          break;
        }
      case 'U': {
          Serial.println(F("Sketch Updated"));
          break;
        }
      case 'F': {
          Serial.println(F("Sketch Update Failure..."));
          break;
        }
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
#ifdef SERVER_DEBUG
  Serial.println(F("\n\n"));
#endif

  return errorOccured;
}
