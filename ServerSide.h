#pragma once

#define SERVER_DEBUG

#define DIRECT_UPDATE
#define ENABLE_WEBSOCKET_SERVER

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>

#ifdef ENABLE_WEBSOCKET_SERVER
  #include <WebSocketsServer.h>
#endif

#ifdef DIRECT_UPDATE
  #include <ESP8266HTTPUpdateServer.h>
#endif

#ifdef ESP8266
extern "C" {
  #include "user_interface.h"  // Used to get Wifi status information
}
#endif

#include "global.h"
#include "WebUpdate.h"
#include "SPIFFS_Params.h"

// https://github.com/bblanchon/ArduinoJson
#include <ArduinoJson.h>

extern bool inAPMode;
extern ESP8266HTTPUpdateServer updateServer;
extern ESP8266WebServer server;
#ifdef ENABLE_WEBSOCKET_SERVER
  extern WebSocketsServer webSocket;    // create a websocket server on port 81
#endif

void ICACHE_FLASH_ATTR beginServerServices();
void ICACHE_FLASH_ATTR connectNetwork();

bool ICACHE_FLASH_ATTR connectWIFI_STA_Mode();
bool ICACHE_FLASH_ATTR connectWIFI_AP_Mode();
void ICACHE_FLASH_ATTR fallbacktoAPMode();

void ICACHE_FLASH_ATTR startServer();
void ICACHE_FLASH_ATTR handleNotFound();

#ifdef ENABLE_WEBSOCKET_SERVER
  void ICACHE_FLASH_ATTR webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t payload_length);
  void ICACHE_FLASH_ATTR startWebSocket();
#endif
void sendStatusUpdate();

bool ICACHE_FLASH_ATTR checkForUpdates();
void ICACHE_FLASH_ATTR restartESP();
