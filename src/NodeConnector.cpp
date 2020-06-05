/*
  Node Connector
  web client based configuration for you IOT project
  Copyright Giedrius Lukosevicius 2020
  MIT License
*/

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

NodeConnector::NodeConnector(uint16_t stateMachineJsonSize) : _configurator("IOT Node"),
                                                              gatewayClient(),
                                                              stateMachineDefinition(stateMachineJsonSize)
{
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

      if (fetchSmdFromGateway())
        saveSmdToFlash();
      else
        loadSmdFromFlash();

      return;
    }
  }

  loadSmdFromFlash();
}

void NodeConnector::startWiFi()
{
  gatewayClient.init(
      _configurator.getParam(PARAM_ACCESS_POINT),
      _configurator.getParam(PARAM_PASSWORD));

  gatewayClient.connect();
}

bool NodeConnector::serveConfigPage()
{
  _configurator.addParam(PARAM_ACCESS_POINT, "");
  _configurator.addParam(PARAM_PASSWORD, "");
  _configurator.addParam(PARAM_IOT_GATEWAY_ADDRESS, "http://192.168.1.123");
  _configurator.addParam(PARAM_NODE_ID, "");
  _configurator.runServer();

  nodeId = _configurator.getParam(PARAM_NODE_ID);
}

bool NodeConnector::fetchSmdFromGateway()
{
  char url[MAX_URL_SIZE];
  strncpy(url, _configurator.getParam(PARAM_IOT_GATEWAY_ADDRESS), MAX_URL_SIZE);
  strncat(url, "/definitions/", MAX_URL_SIZE - strlen(url));
  strncat(url, nodeId, MAX_URL_SIZE - strlen(url));

  WiFiClient *stream = gatewayClient.openMsgPackStream(url);
  if (!stream)
    return false;

  error = deserializeMsgPack(
      stateMachineDefinition,
      *stream,
      DeserializationOption::NestingLimit(JSON_NESTING_LIMIT));

  return isSmdLoaded = error == DeserializationError::Ok;
}

bool NodeConnector::loadSmdFromFlash()
{
  bool success = SPIFFS.begin();
  if (!success)
    return false;

  if (!SPIFFS.exists(SMD_FILE_PATH))
    return false;

  File file = SPIFFS.open(SMD_FILE_PATH, "r");

  error = deserializeMsgPack(
      stateMachineDefinition,
      file,
      DeserializationOption::NestingLimit(JSON_NESTING_LIMIT));

  SPIFFS.end();

  return isSmdLoaded = error == DeserializationError::Ok;
}

bool NodeConnector::saveSmdToFlash()
{
  bool success = SPIFFS.begin();
  if (!success)
    return false;

  File file = SPIFFS.open(SMD_FILE_PATH, "w");

  size_t bitesWritten = serializeMsgPack(stateMachineDefinition, file);

  SPIFFS.end();

  return bitesWritten > 0;
}