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

NodeConnector::NodeConnector(uint16_t stateMachineJsonSize) : _configurator(),
                                                              _hooks(),
                                                              gatewayClient(),
                                                              nodeDefinition(stateMachineJsonSize)
{
}

void NodeConnector::setup(uint16_t waitForPin, bool activateOnHigh, uint16_t waitTimeout)
{
  _configurator.init();

  Serial.println(F("Waiting for signal to start web config..."));
  pinMode(waitForPin, INPUT);
  while (waitTimeout-- > 0)
  {
    delay(1);
    if ((bool)digitalRead(waitForPin) == activateOnHigh)
    {
      Serial.println(F("Serving at http://192.168.1.1"));
      serveConfigPage();
      loadSMD();
      return;
    }
  }

  Serial.println(F("No signal, continue normal load"));
  loadSMD();
}

void NodeConnector::loop(unsigned long timeOut)
{
  if (_getTimeout(_lastCheck) < timeOut)
    return;

  Serial.println(F("Checking SMD for updates"));
  if (fetchSmdFromGateway())
  {
    // to avoid memory leaks and unpredictable state machine status
    // we are going to reset device
    Serial.println(F("Restarting..."));
    delay(100);
    ESP.restart();
    delay(100);
  }
}

bool NodeConnector::initSM(StateMachineController *sm)
{
  if (nodeDefinition.containsKey(NODE_SYNC_OPTIONS))
  {
    JsonVariant smDefinition = nodeDefinition[NODE_STATE_MACHINE];
    sm->setDefinition(smDefinition);

    if (nodeDefinition.containsKey(NODE_STATE_MACHINE))
    {
      JsonVariant syncOptions = nodeDefinition[NODE_SYNC_OPTIONS];
      _hooks.bind(sm, syncOptions);
    }
    return true;
  }
  else
  {
    return false;
  }
}

void NodeConnector::loadSMD()
{
  nodeId = _configurator.getParam(PARAM_NODE_ID);

  if (fetchSmdFromGateway())
    return;

  Serial.println(F("Loading from flash"));
  loadSmdFromFlash();
}

bool NodeConnector::isAccessPointConfigured()
{
  const char *accessPoint = _configurator.getParam(PARAM_ACCESS_POINT);

  Serial.print(F("Access point to use: "));
  Serial.println(accessPoint);
  if (accessPoint && *accessPoint)
  {
    return true;
  }

  return false;
}

void NodeConnector::startWiFi()
{
  if (gatewayClient.isConnected())
    return;

  gatewayClient.init(
      _configurator.getParam(PARAM_ACCESS_POINT),
      _configurator.getParam(PARAM_PASSWORD));

  Serial.println(F("Turning Wifi client On"));
  gatewayClient.on();

  Serial.println(F("Connecting"));
  if (gatewayClient.connect())
  {
    Serial.print(F("Node IP: "));
    Serial.println(gatewayClient.ip);
  }
  else
  {
    Serial.println(F("Connection failed"));
  }
}

bool NodeConnector::serveConfigPage()
{
  // create params and assign defalt values, in case params was not in flash yet
  _configurator.addParam(PARAM_ACCESS_POINT, "");
  _configurator.addParam(PARAM_PASSWORD, "");
  _configurator.addParam(PARAM_IOT_GATEWAY_ADDRESS, "http://192.168.1.123:3000");
  _configurator.addParam(PARAM_NODE_ID, "IOT Node");

  _configurator.runServer(_configurator.getParam(PARAM_NODE_ID), "iot node");
}

bool NodeConnector::fetchSmdFromGateway()
{

  if (!nodeId || !isAccessPointConfigured())
  {
    Serial.println(F("Missing WiFi configuration"));
    return false;
  }

  Serial.println(F("Contacting gateway"));

  startWiFi();

  loadLastModifiedtime();
  _lastCheck = millis();

  char url[MAX_URL_SIZE];
  strncpy(url, _configurator.getParam(PARAM_IOT_GATEWAY_ADDRESS), MAX_URL_SIZE);
  strncat(url, "/definitions/", MAX_URL_SIZE - strlen(url));
  strncat(url, nodeId, MAX_URL_SIZE - strlen(url));

  Serial.print(F("Loading State Machine definition from: "));
  Serial.println(url);

  WiFiClient *stream = gatewayClient.openMsgPackStream(url);
  if (!stream)
    return false;

  Serial.println(F("Deserializing..."));

  error = deserializeMsgPack(
      nodeDefinition,
      *stream,
      DeserializationOption::NestingLimit(JSON_NESTING_LIMIT));

  Serial.print(F("Deserialize status: "));
  Serial.println(error.c_str());

  saveLastModifiedTime();

  isSmdLoaded = error == DeserializationError::Ok;

  if (isSmdLoaded)
  {
    Serial.println(F("Done. Saving SMD to flash"));
    saveSmdToFlash();
  }

  return isSmdLoaded;
}

bool NodeConnector::loadSmdFromFlash()
{
  Serial.println(F("Loading SMD from flash drive"));

  bool success = SPIFFS.begin();
  if (!success)
    return false;

  Serial.print(F("Total flash space: "));
  Serial.println(SPIFFS.totalBytes());

  if (!SPIFFS.exists(SMD_FILE_PATH))
    return false;

  File file = SPIFFS.open(SMD_FILE_PATH, "r");

  int size = file.size();
  Serial.print(F("File size: "));
  Serial.println(size);

  error = deserializeMsgPack(
      nodeDefinition,
      file,
      DeserializationOption::NestingLimit(JSON_NESTING_LIMIT));

  file.close();

  SPIFFS.end();

  Serial.print(F("Deserialize status: "));
  Serial.println(error.c_str());

  return isSmdLoaded = error == DeserializationError::Ok;
}

bool NodeConnector::saveSmdToFlash()
{
  bool success = SPIFFS.begin(true); // true -> formateOnFail
  if (!success)
    return false;

  File file = SPIFFS.open(SMD_FILE_PATH, "w");

  size_t bitesWritten = serializeMsgPack(nodeDefinition, file);

  file.close();

  SPIFFS.end();

  Serial.print(F("Saved bytes: "));
  Serial.println(bitesWritten);

  return bitesWritten > 0;
}

void NodeConnector::saveLastModifiedTime()
{
  bool success = SPIFFS.begin();
  if (!success)
    return;

  if (gatewayClient.timeStamp)
  {

    File file = SPIFFS.open(LAST_MODIFIED_FILE_PATH, "w");
    file.write((uint8_t *)gatewayClient.timeStamp, strlen(gatewayClient.timeStamp));
    file.close();

    Serial.print(F("Updated SMD date: "));
    Serial.println(gatewayClient.timeStamp);
  }
  else
  {
    SPIFFS.remove(LAST_MODIFIED_FILE_PATH);
  }

  SPIFFS.end();
}

void NodeConnector::loadLastModifiedtime()
{
  bool success = SPIFFS.begin();
  if (!success)
    return;

  if (!SPIFFS.exists(LAST_MODIFIED_FILE_PATH))
    return;

  File file = SPIFFS.open(LAST_MODIFIED_FILE_PATH, "r");
  file.read((uint8_t *)gatewayClient.timeStamp, sizeof(gatewayClient.timeStamp));
  file.close();

  Serial.print(F("Last SMD date: "));
  Serial.println(gatewayClient.timeStamp);

  SPIFFS.end();
}

unsigned long NodeConnector::_getTimeout(unsigned long start)
{
  return diff(start, millis());
}