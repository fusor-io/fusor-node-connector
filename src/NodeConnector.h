/*
  Wifi Configurator -
  web client based configuration for you IOT project
  Copyright Giedrius Lukosevicius 2020
  MIT License
*/

#ifndef nodeconnector_h
#define nodeconnector_h

#include <ArduinoJson.h>
// See: https://arduinojson.org/v6/api/

#include "WiFiClient/WiFiClient.h"

#define MAX_CONNECT_RETRY 10

const char PARAM_ACCESS_POINT[] = "access_point";
const char PARAM_PASSWORD[] = "password";
const char PARAM_IOT_GATEWAY_ADDRESS[] = "IOT_gateway_address";
const char PARAM_NODE_ID[] = "node_ID";

class NodeConnector
{

public:
  NodeConnector(const char *);

  void serveConfigPage();
  void setup(uint16_t, bool activateOnHigh = false, uint16_t waitTimeout = 3000);
  void readSMD();

  // wifi client related
  void startWiFi();
  WiFiClient wifiClient;

private:
  const char *_nodeId;

  WifiConfigurator _configurator;
};

#endif
