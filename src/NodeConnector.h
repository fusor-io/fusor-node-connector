/*
  Wifi Configurator -
  web client based configuration for you IOT project
  Copyright Giedrius Lukosevicius 2020
  MIT License
*/

#ifndef nodeconnector_h
#define nodeconnector_h

#include <Arduino.h>
#ifdef ESP32
#include <SPIFFS.h>
#else
#include <FS.h>
#endif
// https://github.com/espressif/arduino-esp32/blob/master/libraries/SPIFFS/src/SPIFFS.cpp

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#else
#include <WiFi.h>
#include <HTTPClient.h>
#endif

#include <ArduinoJson.h>
// See: https://arduinojson.org/v6/api/

#include <WifiConfigurator.h>
#include <StateMachine.h>

#include "GatewayClient/GatewayClient.h"
#include "SMHooks/SMHooks.h"

#define MAX_CONNECT_RETRY 10
#define DEFAULT_STATEM_MACHINE_JSON_SIZE 4096
#define MAX_URL_SIZE 256
#define JSON_NESTING_LIMIT 20

const char PARAM_ACCESS_POINT[] = "access_point";
const char PARAM_PASSWORD[] = "password";
const char PARAM_IOT_GATEWAY_ADDRESS[] = "IOT_gateway_address";
const char PARAM_NODE_ID[] = "node_ID";
const char SMD_FILE_PATH[] = "/smd.mpk";
const char LAST_MODIFIED_FILE_PATH[] = "/mod.txt";

class NodeConnector
{

public:
  NodeConnector(uint16_t stateMachineJsonSize = DEFAULT_STATEM_MACHINE_JSON_SIZE);

  bool serveConfigPage();
  void setup(uint16_t, bool activateOnHigh = false, uint16_t waitTimeout = 3000);
  void loop(unsigned long timeOut = 60000);
  void loadSMD();
  void initSM(StateMachineController *);

  bool fetchSmdFromGateway();
  bool loadSmdFromFlash();
  bool saveSmdToFlash();

  void saveLastModifiedTime();
  void loadLastModifiedtime();

  bool isAccessPointConfigured();

  bool isSmdLoaded = false;

  // wifi client related
  void startWiFi();
  GatewayClient gatewayClient;

  const char *nodeId;

  DynamicJsonDocument stateMachineDefinition;
  DeserializationError error;

private:
  WifiConfigurator _configurator;
  SMHooks _hooks;

  unsigned long _lastCheck;
  unsigned long _getTimeout(unsigned long);
};

#endif
