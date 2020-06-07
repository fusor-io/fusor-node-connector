#ifndef gatewayclient_h
#define gatewayclient_h

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#else
#include <WiFi.h>
#include <HTTPClient.h>
#endif

#define MAX_CONNECT_RETRY 20
#define HTTP_TIME_STAMP_LENGTH 30
// ex. "Wed, 21 Oct 2015 07:28:00 GMT" + /0

// #define DEFAULT_STATE_MACHINE_SIZE 4096
// #define JSON_NESTING_LIMIT 20

const char HEADER_ACCEPT[] = "accept";
const char HEADER_IF_MODIFIED_SINCE[] = "if-modified-since";

const char CONTENT_TYPE_MSG_PACK[] = "application/msgpack";

class GatewayClient
{
public:
  GatewayClient();
  void init(const char *, const char *);
  bool connect();
  void off();
  void on();
  bool isConnected();

  WiFiClient *openMsgPackStream(const char *);
  void closeMsgPackStream();

  char ip[16];

  // see https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Date
  // see https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Last-Modified
  // see https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/If-Modified-Since
  // <day-name>, <day> <month> <year> <hour>:<minute>:<second> GMT
  char timeStamp[HTTP_TIME_STAMP_LENGTH];

private:
  const char *_ssid;
  const char *_password;

  HTTPClient _http;
};

#endif
