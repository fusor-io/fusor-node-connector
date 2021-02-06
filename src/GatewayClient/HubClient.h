#ifndef hubclient_h
#define hubclient_h

#include <ArduinoJson.h>

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#else
#include <WiFi.h>
#include <HTTPClient.h>
#endif

#include "../LocalTimeHandler/LocalTimeHandler.h"

#define MAX_CONNECT_RETRY 20
#define HTTP_TIME_STAMP_LENGTH 30
// ex. "Wed, 21 Oct 2015 07:28:00 GMT" + /0

const char HEADER_CONTENT_TYPE[] = "content-type";
const char HEADER_ACCEPT[] = "accept";
const char HEADER_IF_MODIFIED_SINCE[] = "if-modified-since";

const char CONTENT_TYPE_MSG_PACK[] = "application/msgpack";

class HubClient
{
public:
  HubClient();
  void init(const char *, const char *);
  bool connect();
  void off();
  void on();
  bool isConnected();
  bool ensureConnection();

  WiFiClient *openMsgPackStream(const char *, const char *);
  void closeMsgPackStream();

  void postMsgPack(const char *, const uint8_t *, size_t);

  char ip[16];

  // see https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Date
  // see https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Last-Modified
  // see https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/If-Modified-Since
  // <day-name>, <day> <month> <year> <hour>:<minute>:<second> GMT
  char timeStamp[HTTP_TIME_STAMP_LENGTH];

private:
  const char *_ssid;
  const char *_password;

  // see https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266HTTPClient/src/ESP8266HTTPClient.cpp
  HTTPClient _http;

  LocalTimeHandler _localTimeHandler;
};

#endif
