#pragma once

#include "Ntp.h"
#include "global.h"
#include <FS.h>
#include <ArduinoJson.h>

#define PARAMS_FILE "/config.json"

void ICACHE_FLASH_ATTR beginSPIFFS();
bool ICACHE_FLASH_ATTR readSPIFFS_Settings();
bool ICACHE_FLASH_ATTR saveSPIFFS_Settings();

void ICACHE_FLASH_ATTR parseConfig(JsonObject& settings);

void ICACHE_FLASH_ATTR parseGeneral(JsonObject& general);
void ICACHE_FLASH_ATTR parseNetwork(JsonObject& network);
void ICACHE_FLASH_ATTR parseMQTT(JsonObject& mqtt);
void ICACHE_FLASH_ATTR parseNTP(JsonObject& json_ntp);
void ICACHE_FLASH_ATTR parseTransitions(JsonObject& transition);

void ICACHE_FLASH_ATTR format();
bool ICACHE_FLASH_ATTR removeParams();
