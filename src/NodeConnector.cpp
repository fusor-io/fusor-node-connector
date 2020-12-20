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
 * Usage:
 *  - call `setup` and then `initSM` methods from Arduino `setup` function
 *  - call `loop` from Arduino program `loop` function
 */

NodeConnector::NodeConnector(uint16_t stateMachineJsonSize, uint16_t paramStoreJsonSize) : _configurator(),
                                                                                           _hooks(),
                                                                                           _syncInConfig(),
                                                                                           gatewayClient(),
                                                                                           nodeDefinition(stateMachineJsonSize),
                                                                                           paramStore(paramStoreJsonSize)
{
}

/**
 * Initialize Node, by loading configurations
 * IMPORTANT: should be called from Arduino program `setup` function
 * @param waitForPin input pin number to watch for signal if configuration needed
 * @param activateOnHigh if true - low to high pin signal change should activate configuration
 * @param waitTimeout how long to wait for signal
 */
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
      loadDefinition();
      return;
    }
  }

  Serial.println(F("No signal, continue normal load"));
  loadDefinition();
}

/**
 * Performs definition and params sync with Gateway
 * IMPORTANT: should be called from Arduino program `loop` function (each time)
 * @param timeOut minimal wait time for sync to happen
 */
void NodeConnector::loop(unsigned long timeOut)
{
  if (_getTimeout(_lastCheck) < timeOut)
    return;

  Serial.println(F("Checking definition for updates"));
  if (fetchDefinitionFromGateway())
  {
    // to avoid memory leaks and unpredictable state machine status
    // we are going to reset device
    Serial.println(F("Restarting..."));
    delay(100);
    ESP.restart();
    delay(100);
  }

  // TODO: implement reading of params from the Gateway
}

/**
 * Loads definition to a provided State Machine and hooks to its life cycle
 * IMPORTANT: should be called from Arduino program `setup` function
 */
bool NodeConnector::initSM(StateMachineController *sm)
{
  // SYNC OUT options defines how data should flow from the state machine to the gateway
  // SYNC IN options defines how params should flow from the gateway to the state machine

  if (nodeDefinition.containsKey(NODE_STATE_MACHINE))
  {
    JsonVariant smDefinition = nodeDefinition[NODE_STATE_MACHINE];
    sm->setDefinition(smDefinition);

    // Bind to State Machine for outward data flow
    if (nodeDefinition.containsKey(NODE_SYNC_OUT_OPTIONS))
    {
      JsonVariant syncOutOptions = nodeDefinition[NODE_SYNC_OUT_OPTIONS];
      _initPostUrl();

      // Hook into State Machine data update cycle
      // Var updates in State Machine will fire posts to the gateway,
      // according to sync options
      _hooks.init(&gatewayClient, _postUrl, sm, syncOutOptions);
    }

    // Bind to State Machine for inward data flow
    if (nodeDefinition.containsKey(NODE_SYNC_IN_OPTIONS))
    {
      JsonVariant syncInOptions = nodeDefinition[NODE_SYNC_IN_OPTIONS];
      // TODO:
      //   use SynInOption
      //   find a way to hook into SM
      // ?: could we read and write in one request?
    }

    return true;
  }
  else
  {
    return false;
  }
}

/**
 * Read definition from Wifi on device startup
 * Fallback to FLASH version of definition if Wifi fails
 */
void NodeConnector::loadDefinition()
{
  // Use Wifi configurator service to read node and gateway params
  // Configurator provides web UI for setting IOT Node params like
  // hotspot name and password, Node identifier, Gateway url
  nodeId = _configurator.getParam(PARAM_NODE_ID);
  _gatewayAddress = _configurator.getParam(PARAM_IOT_GATEWAY_ADDRESS);

  if (fetchDefinitionFromGateway())
    return;

  Serial.println(F("Loading from flash"));
  loadDefinitionFromFlash();

  _initGetUrl();
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

/**
 * Read definition using Wifi from Gateway server
 */
bool NodeConnector::fetchDefinitionFromGateway()
{

  if (!_openWiFiConnection())
    return false;

  loadLastModifiedtime();
  _lastCheck = millis();

  char url[MAX_URL_SIZE];
  strncpy(url, _gatewayAddress, MAX_URL_SIZE);
  strncat(url, ENDPOINT_DEFINITIONS, MAX_URL_SIZE - strlen(url));
  strncat(url, nodeId, MAX_URL_SIZE - strlen(url));

  Serial.print(F("Loading node definition"));

  isSmdLoaded = _fetchMsgPack(url, nodeDefinition);

  saveLastModifiedTime();

  if (isSmdLoaded)
  {
    Serial.println(F("Done. Saving node definition to flash"));
    saveSmdToFlash();
  }

  return isSmdLoaded;
}

/**
 * Read definition from FLASH memory chip
 */
bool NodeConnector::loadDefinitionFromFlash()
{
  Serial.println(F("Loading definition from flash drive"));

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

/**
 * Save definition to FLASH memory chip
 */
bool NodeConnector::saveSmdToFlash()
{
  bool success = SPIFFS.begin(true); // true -> formatOnFail
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

/**
 * Save most recent definition version modification time.
 * Later it will be used to check if new definition version is available
 * on Gateway server.
 */
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

/**
 * Load most recent definition version modification time from FLASH
 * Then it can be used to check if new definition version is available
 * on Gateway server.
 */
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

/**
 * Read global params from Gateway server
 */
bool NodeConnector::fetchParamsFromGateway()
{
  if (!_openWiFiConnection())
    return false;

  if (_fetchMsgPack(_getUrl, paramStore, 1))
  {
    if (!paramStore.is<JsonObject>())
      return false;

    JsonObject params = paramStore.as<JsonObject>();

    for (JsonObject::iterator it = params.begin(); it != params.end(); ++it)
    {
      JsonVariant value = it->value();
      
      // is<float> is ok for integers too
      if (value.is<float>())
        _hooks.setVar(it->key().c_str(), value.as<float>());
    }

    return true;
  }

  return false;

  // TODO: find good way for preserving values during restart
  // we cant write to FLASH every time as it has limited rewrites capacity
  // Separate config for StateMachine store preservation ???
}

/**
 * Given url and target JsonDocument read data from Gateway using MsgPack as a content type
 */
bool NodeConnector::_fetchMsgPack(const char *url, DynamicJsonDocument target, uint8_t nestingLimit)
{
  Serial.print(F("Reading from: "));
  Serial.println(_getUrl);

  WiFiClient *stream = gatewayClient.openMsgPackStream(url);
  if (!stream)
    return false;

  Serial.println(F("Deserializing..."));

  error = deserializeMsgPack(
      target,
      *stream,
      DeserializationOption::NestingLimit(nestingLimit));

  Serial.print(F("Deserialize status: "));
  Serial.println(error.c_str());

  return error == DeserializationError::Ok;
}

/**
 * Build url for posting Node result values (eg. senor readings)
 */
void NodeConnector::_initPostUrl()
{
  size_t urlSize = strlen(_gatewayAddress) + strlen(ENDPOINT_NODE) + strlen(nodeId) + strlen(ENDPOINT_PARAM_BATCH) + 1;
  _postUrl = (const char *)new char[urlSize];
  strcpy((char *)_postUrl, _gatewayAddress);
  strcat((char *)_postUrl, ENDPOINT_NODE);
  strcat((char *)_postUrl, nodeId);
  strcat((char *)_postUrl, ENDPOINT_PARAM_BATCH);
}

/**
 * Build url for geting global params or values from other Nodes
 */
void NodeConnector::_initGetUrl()
{
  if (nodeDefinition.containsKey(NODE_SYNC_IN_OPTIONS))
  {
    JsonVariant syncInOptions = nodeDefinition[NODE_SYNC_IN_OPTIONS];
    _syncInConfig.init(syncInOptions, _gatewayAddress);
    _getUrl = _syncInConfig.requestUrl;
  }
}

/**
 * Check if Access Point configuration is available and start WiFi
 */
bool NodeConnector::_openWiFiConnection()
{
  if (!nodeId || !isAccessPointConfigured())
  {
    Serial.println(F("Missing WiFi configuration"));
    return false;
  }

  Serial.println(F("Contacting gateway"));

  startWiFi();

  return true;
}

/**
 * Check how much milliseconds passed from the provided time
 * It also handles time counter overflow condition
 */
unsigned long NodeConnector::_getTimeout(unsigned long start)
{
  return diff(start, millis());
}