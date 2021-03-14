/*
  Fusor Node Connector
  Copyright Giedrius Lukosevicius 2020
  MIT License
*/

#include "./PrintWrapper/PrintWrapper.h"
#include "NodeConnector.h"

#ifdef SM_DEBUGGER
__debugPrinter = _nc_debugPrinter;
#endif

/**
 * Setup features:
 *  - provide web UI to configure connection to WiFi station and Fusor Hub
 *  - fetch State Machine Definition (SMD) from Fusor Hub
 *  - save fetched SMD to FLASH memory
 *  - load SMD from FLASH on startup
 * 
 * Loop features:
 *  - read params from Fusor Hub
 *  - post params to Fusor Hub 
 *  
 * Usage:
 *  - call `setup` and then `initSM` methods from Arduino `setup` function
 *  - call `loop` from Arduino program `loop` function
 */

NodeConnector::NodeConnector(
    const char *nodeId,
    const char *configPassword,
    uint16_t stateMachineJsonSize,
    uint16_t paramStoreJsonSize) : _configurator(),
                                   _hooks(),
                                   _syncInConfig(),
                                   _persistentStorage(),
                                   hubClient(),
                                   nodeDefinition(stateMachineJsonSize),
                                   paramStore(paramStoreJsonSize),
                                   fs(),
                                   sm(nodeId, _nc_sleepFunction, _nc_getTime)
{
  _nodeId = nodeId;
  _configPassword = configPassword;
}

/**
 * Initialize Node, by loading configurations
 * IMPORTANT: should be called from Arduino program `setup` function
 * @param waitForPin input pin number to watch for signal if configuration needed
 * @param activateOnHigh if true - low to high pin signal change should activate configuration
 * @param waitTimeout how long to wait for signal
 */
bool NodeConnector::setup(uint16_t waitForPin, bool activateOnHigh, uint16_t waitTimeout)
{

#ifdef SM_DEBUGGER
  sm.setDebugPrinter(_nc_debugPrinter);
#endif

  _configurator.init();

  Serial << F("Waiting for signal to start web config...\n");
  pinMode(waitForPin, INPUT);
  while (waitTimeout-- > 0)
  {
    delay(1);
    if ((bool)digitalRead(waitForPin) == activateOnHigh)
    {
      Serial << F("Serving at http://192.168.1.1\n");
      serveConfigPage();
      loadDefinition();
      return _initSM();
    }
  }

  Serial << F("No signal, continue normal load\n");

  loadDefinition();
  return _initSM();
}

/**
 * Starts State Machine
 * IMPORTANT: should be called from Arduino program `setup` function,
 * after registering actions and plugins
 */
void NodeConnector::start()
{
  sm.init();
}

/**
 * Performs definition and params sync with Fusor Hub
 * IMPORTANT: should be called from Arduino program `loop` function (each time)
 * @param timeOut minimal wait time for sync to happen
 */
void NodeConnector::loop(unsigned long timeOut)
{
  sm.cycle();

  if (getTimeout(_lastTimeDefinitionChecked) >= timeOut)
  {
    Serial << F("Checking definition for updates\n");
    if (fetchDefinitionFromHub())
    {
      // TODO: do something if storeSmd fails
      if (storeSmd())
      {
        // to avoid memory leaks and unpredictable state machine status
        // we are going to reset device

        _persistentStorage.saveOnReboot();

        Serial << F("Restarting...\n");
        delay(100);
        ESP.restart();
        delay(100);
      }
    }
  }

  if (getTimeout(_lastTimeSyncInAttempted) >= _syncInConfig.delay)
    fetchParamsFromHub();
}

/**
 * Read definition from Wifi on device startup
 * Fallback to FLASH version of definition if Wifi fails
 */
void NodeConnector::loadDefinition()
{
  // Use Wifi configurator service to read node and hub connectivity params
  // Configurator provides web UI for setting IOT Node params like
  // hotspot name and password, Node identifier, Hub url
  nodeId = _configurator.getParam(PARAM_NODE_ID);
  _hubAddress = _configurator.getParam(PARAM_FUSOR_HUB_ADDRESS);

  if (fetchDefinitionFromHub())
    storeSmd();
  else
  {
    Serial << F("Loading from flash\n");
    loadDefinitionFromFlash();
  }

  _initGetUrl();
}

bool NodeConnector::isAccessPointConfigured()
{
  const char *accessPoint = _configurator.getParam(PARAM_ACCESS_POINT);

  Serial << F("Access point to use: ") << accessPoint << "\n";
  return accessPoint && *accessPoint;
}

void NodeConnector::startWiFi()
{
  if (hubClient.isConnected())
    return;

  hubClient.init(
      _configurator.getParam(PARAM_ACCESS_POINT),
      _configurator.getParam(PARAM_PASSWORD));

  Serial << F("Turning Wifi client On\n");
  hubClient.on();

  Serial << F("Connecting\n");
  if (hubClient.connect())
  {
    Serial << F("Node IP: ") << hubClient.ip << "\n";
  }
  else
  {
    Serial << F("Connection failed\n");
  }
}

bool NodeConnector::serveConfigPage()
{
  // create params and assign defalt values, in case params was not in flash yet
  _configurator.addParam(PARAM_ACCESS_POINT, "");
  _configurator.addParam(PARAM_PASSWORD, "");
  _configurator.addParam(PARAM_FUSOR_HUB_ADDRESS, "http://192.168.1.123:3000");
  _configurator.addParam(PARAM_NODE_ID, _nodeId);

  _configurator.runServer(_configurator.getParam(PARAM_NODE_ID), _configPassword);
}

/**
 * Read definition using Wifi from the Fusor Hub
 */
bool NodeConnector::fetchDefinitionFromHub()
{

  if (!_openWiFiConnection())
    return false;

  const char *_definitionLastUpdatedAt = loadLastModifiedtime();

  // We save last attempt time (not success time)
  // This is to prevent slowing down node due to excessive connections to the hub
  // on each loop in case we lost connectivity
  _lastTimeDefinitionChecked = millis();

  char url[MAX_URL_SIZE];
  strncpy(url, _hubAddress, MAX_URL_SIZE);
  strncat(url, ENDPOINT_DEFINITIONS, MAX_URL_SIZE - strlen(url));
  strncat(url, nodeId, MAX_URL_SIZE - strlen(url));

  Serial << F("Loading node definition\n");

  isSmdLoaded = _fetchMsgPack(url, &nodeDefinition, _definitionLastUpdatedAt);

  Serial << (isSmdLoaded ? F("Done. Saving node definition to flash\n") : F("Definition not loaded\n"));

  return isSmdLoaded;
}

/**
 * Store SMD and its timestamp
 */
bool NodeConnector::storeSmd()
{
  bool savedSmd = saveSmdToFlash();
  bool savedTime = saveLastModifiedTime(_timeStampBuff);
  return savedSmd && savedTime;
}

/**
 * Read definition from FLASH memory chip
 */
bool NodeConnector::loadDefinitionFromFlash()
{
  Serial << F("Loading definition from flash drive\n");

  bool success = fs.begin();
  if (!success)
    return false;

  Serial << F("Total flash space: ") << fs.totalBytes() << "\n";

  if (!fs.exists(SMD_FILE_PATH))
    return false;

  File file = fs.open(SMD_FILE_PATH, "r");

  int size = file.size();
  Serial << F("File size: ") << size << "\n";

  error = deserializeMsgPack(
      nodeDefinition,
      file,
      DeserializationOption::NestingLimit(JSON_NESTING_LIMIT));

  file.close();

  fs.end();

  Serial << F("Deserialize status: ") << error.c_str() << "\n";

  return isSmdLoaded = error == DeserializationError::Ok;
}

/**
 * Save definition to FLASH memory chip
 */
bool NodeConnector::saveSmdToFlash()
{
  bool success = fs.begin(true);
  if (!success)
  {
    Serial << F("Failed saving\n");
    return false;
  }

  File file = fs.open(SMD_FILE_PATH, "w");

  size_t bitesWritten = serializeMsgPack(nodeDefinition, file);

  file.close();

  fs.end();

  Serial << F("Saved bytes: ") << bitesWritten << "\n";

  return bitesWritten > 0;
}

/**
 * Save most recent definition version modification time.
 * Later it will be used to check if new definition version is available
 * on the Fusor Hub.
 */
bool NodeConnector::saveLastModifiedTime(const char *timeStamp)
{
  bool success = fs.begin();
  if (!success)
    return false;

  if (timeStamp && timeStamp[0])
  {

    File file = fs.open(LAST_MODIFIED_FILE_PATH, "w");
    file.write((uint8_t *)timeStamp, strlen(timeStamp));
    file.close();

    Serial << F("Updated SMD date: ") << timeStamp << "\n";
  }
  else
  {
    fs.remove(LAST_MODIFIED_FILE_PATH);
  }

  fs.end();
  return true;
}

/**
 * Load most recent definition version modification time from FLASH
 * Then it can be used to check if new definition version is available
 * on the Fusor Hub.
 */
const char *NodeConnector::loadLastModifiedtime()
{
  bool success = fs.begin();
  if (!success)
    return nullptr;

  if (!fs.exists(LAST_MODIFIED_FILE_PATH))
    return nullptr;

  File file = fs.open(LAST_MODIFIED_FILE_PATH, "r");
  file.read((uint8_t *)_timeStampBuff, sizeof(_timeStampBuff));
  file.close();

  Serial << F("Last SMD date: ") << _timeStampBuff << "\n";

  fs.end();

  return _timeStampBuff;
}

/**
 * Read global params from the Fusor Hub
 */
bool NodeConnector::fetchParamsFromHub()
{
  if (!_getUrl)
    return false;

  if (!_openWiFiConnection())
    return false;

  // We save last attempt time (not success time)
  // This is to prevent slowing down node due to excessive connections to the hub
  // on each loop in case we lost connectivity
  _lastTimeSyncInAttempted = millis();

  if (_fetchMsgPack(_getUrl, &paramStore, nullptr, 1))
  {
    if (!paramStore.is<JsonObject>())
      return false;

    JsonObject params = paramStore.as<JsonObject>();

    for (JsonObject::iterator it = params.begin(); it != params.end(); ++it)
    {
      JsonVariant value = it->value();

      if (value.is<long int>())
        _hooks.setVar(it->key().c_str(), value.as<long int>());
      else if (value.is<float>())
        _hooks.setVar(it->key().c_str(), value.as<float>());
    }

    return true;
  }

  return false;
}

void NodeConnector::disbaleSerialPrint()
{
  __nc_serial_enabled = false;
}

/**
 * Loads definition to a provided State Machine and hooks to its life cycle
 */
bool NodeConnector::_initSM()
{
  if (!isSmdLoaded)
    return false;

  // SYNC OUT options defines how data should flow from the state machine to the hub
  // SYNC IN options defines how params should flow from the hub to the state machine

  if (nodeDefinition.containsKey(NODE_STATE_MACHINE))
  {
    JsonVariant smDefinition = nodeDefinition[NODE_STATE_MACHINE];
    sm.setDefinition(smDefinition);

    // Bind to State Machine for outward data flow
    if (nodeDefinition.containsKey(NODE_SYNC_OUT_OPTIONS))
    {
      JsonVariant syncOutOptions = nodeDefinition[NODE_SYNC_OUT_OPTIONS];
      _initPostUrl();

      // Hook into State Machine data update cycle
      // Var updates in State Machine will fire posts to the hub,
      // according to sync options
      _hooks.init(&hubClient, &_persistentStorage, _postUrl, &sm, syncOutOptions);
    }

    // Bind to Persistent Storage to the State Machine and read variable values
    if (nodeDefinition.containsKey(NODE_PERSISTENT_STORAGE))
    {
      _persistentStorage.init(nodeDefinition[NODE_PERSISTENT_STORAGE], &(sm.compute.store));

      // Load initial variable values from the store (or set to defaults by config)
      _persistentStorage.load();
    }

    // Bind to State Machine for inward data flow
    if (nodeDefinition.containsKey(NODE_SYNC_IN_OPTIONS))
    {
      JsonVariant syncInOptions = nodeDefinition[NODE_SYNC_IN_OPTIONS];

      // Try to overwrite initial variable values from the hub
      // If that fails, we can still have values from the Persistent Storage (see prev step above)
      if (fetchParamsFromHub())
        Serial << F("Node params loaded\n");
    }

    _addFunctions();

    return true;
  }
  else
  {
    return false;
  }
}

/**
 * Given url and target JsonDocument read data from Fusor Hub using MsgPack as a content type
 */
bool NodeConnector::_fetchMsgPack(const char *url, DynamicJsonDocument *target, const char *ifModifiedSince, uint8_t nestingLimit)
{
  Serial << F("Reading from: ") << url << "\n";

  WiFiClient *stream = hubClient.openMsgPackStream(url, ifModifiedSince);
  if (!stream)
    return false;

  if (hubClient.timeStamp[0])
    strcpy(_timeStampBuff, hubClient.timeStamp);
  else
    _timeStampBuff[0] = '\0';

  Serial << F("Deserializing...\n");

  error = deserializeMsgPack(
      *target,
      *stream,
      DeserializationOption::NestingLimit(nestingLimit));

  Serial << F("Deserialize status: ") << error.c_str() << "\n";

  return error == DeserializationError::Ok;
}

/**
 * Build url for posting Node result values (eg. senor readings)
 */
void NodeConnector::_initPostUrl()
{
  size_t urlSize = strlen(_hubAddress) + strlen(ENDPOINT_NODE) + strlen(nodeId) + strlen(ENDPOINT_PARAM_BATCH) + 1;
  _postUrl = (const char *)new char[urlSize];
  strcpy((char *)_postUrl, _hubAddress);
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
    _syncInConfig.init(syncInOptions, _hubAddress);
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
    Serial << F("Missing WiFi configuration\n");
    return false;
  }

  Serial << F("Contacting hub\n");

  startWiFi();

  return true;
}

/**
 * Attach date/time functions to a State Machine
 */
void NodeConnector::_addFunctions()
{
  // all time is in GMT timezone
  sm.registerFunction("month", _nc_month);
  sm.registerFunction("weekday", _nc_weekDay);
  sm.registerFunction("day", _nc_day);
  sm.registerFunction("hour", _nc_hour);
}

/**
 * Helper functions for binding State Machine to Arduino environment
 */

void _nc_sleepFunction(unsigned long ms)
{
  delay(ms);
}

unsigned long _nc_getTime()
{
  return millis();
}

void _nc_debugPrinter(const char *message)
{
  Serial << message;
}

/**
 * State Machine extension MathFunctions.
 * All should return float and accept params as ActionContext.
 * Date/time functions used here comes from TimeLib.h
 * @see: https://github.com/PaulStoffregen/Time/blob/master/TimeLib.h
 */

VarStruct _nc_month(ActionContext *ctx)
{
  return month(_nc_localTime(ctx));
}

VarStruct _nc_day(ActionContext *ctx)
{
  return day(_nc_localTime(ctx));
}

VarStruct _nc_weekDay(ActionContext *ctx)
{
  return weekday(_nc_localTime(ctx));
}

VarStruct _nc_hour(ActionContext *ctx)
{
  return hour(_nc_localTime(ctx));
}

/**
 * Get time in local time zone
 * @note It is expected `@hub.tz` to be available and to contain time zone offset in hours
 * If is is not set on the Hub or not listed among inbound fields config, 
 * then time will be in GMT
 */
time_t _nc_localTime(ActionContext *ctx)
{
  float tz = ctx->compute->getVarFloat("@hub.tz");
  return now() + (time_t)(round((tz * 3600.0f)));
}