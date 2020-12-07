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
#include "SyncInOptions/SyncInOptions.h"
#include "Utils/Utils.h"

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
const char ENDPOINT_DEFINITIONS[] = "/definitions/sm/";
const char ENDPOINT_NODE[] = "/node/";
const char ENDPOINT_PARAM_BATCH[] = "/batch";

#define NODE_SYNC_OUT_OPTIONS "o"
#define NODE_SYNC_IN_OPTIONS "i"
#define NODE_STATE_MACHINE "s"

/*
 * Structure of Node Definition JSON
 * 
 * {
 *   "o": { "field_name_1": <sync options>,  ... } - see SyncOutElementConfig.h for outbound sync options details
 *   "i": { ... } - see SyncInOptions.h for inbound data options (params to read from gateway)
 *   "s": <state machine definition> 
 * }
 * 
 */

class NodeConnector
{

public:
  NodeConnector(uint16_t stateMachineJsonSize = DEFAULT_STATEM_MACHINE_JSON_SIZE);

  bool serveConfigPage();
  void setup(uint16_t, bool activateOnHigh = false, uint16_t waitTimeout = 3000);
  void loop(unsigned long timeOut = 60000);
  void loadDefinition();
  bool initSM(StateMachineController *);

  bool fetchDefinitionFromGateway();
  bool loadDefinitionFromFlash();
  bool saveSmdToFlash();

  void saveLastModifiedTime();
  void loadLastModifiedtime();

  bool isAccessPointConfigured();

  bool isSmdLoaded = false;

  // wifi client related
  void startWiFi();
  GatewayClient gatewayClient;

  const char *nodeId;

  DynamicJsonDocument nodeDefinition;
  JsonVariant stateMachine;
  JsonVariant syncOptions;
  DeserializationError error;


private:
  WifiConfigurator _configurator;
  SMHooks _hooks;
  SyncInOptions _syncInConfig;
  
  const char *_gatewayAddress;
  const char *_postUrl; // url to post Node results (eg. sensor data)
  const char *_getUrl;  // url to get Node inputs (eg. configurations or results of other Nodes)
  void _initPostUrl();
  void _initGetUrl();

  unsigned long _lastCheck;
  unsigned long _getTimeout(unsigned long);
};

#endif
