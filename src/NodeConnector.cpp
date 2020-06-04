/*
  Node Connector
  web client based configuration for you IOT project
  Copyright Giedrius Lukosevicius 2020
  MIT License
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WifiConfigurator.h>

#include "NodeConnector.h"

/**
 * Setup features:
 *  - provide web UI to configure connection to WiFi station and IOT Gateway
 *  - fetch State Machine Definition (SMD) from IOT Gateway
 *  - save fetched SMD to SPIFFS
 *  - load SMD from SPIFFS on startup
 * 
 * Loop features:
 *  - read params from IOT Gateway
 *  - post params to IOT Gateway 
 *  
 */

NodeConnector::NodeConnector(const char *id) : _configurator(id), WiFiClient()
{
  _nodeId = id;
}

void NodeConnector::setup(uint16_t waitForPin, bool activateOnHigh, uint16_t waitTimeout)
{
  pinMode(waitForPin, INPUT);
  while (waitTimeout-- > 0)
  {
    delay(1);
    if ((bool)digitalRead(waitForPin) == activateOnHigh)
    {
      serveConfigPage();
      return;
    }
  }
}

void NodeConnector::startWiFi()
{
  wifiClient.init(
      _configurator.getParam(PARAM_ACCESS_POINT),
      _configurator.getParam(PARAM_PASSWORD));

  wiFiClient.connect();
}

void NodeConnector::serveConfigPage()
{
  _configurator.addParam(PARAM_ACCESS_POINT, "");
  _configurator.addParam(PARAM_PASSWORD, "");
  _configurator.addParam(PARAM_IOT_GATEWAY_ADDRESS, "");
  _configurator.addParam(PARAM_NODE_ID, "");
  _configurator.runServer();
}
