#include <Arduino.h>

#include "GatewayClient.h"

GatewayClient::GatewayClient()
{
}

void GatewayClient::init(const char *ssid, const char *password)
{
  _ssid = ssid;
  _password = password;
  _timeStamp[0] = 0;
}

void GatewayClient::off()
{
  WiFi.mode(WIFI_OFF);
  // WiFi.forceSleepBegin();
  delay(1);
}

void GatewayClient::on()
{
  // WiFi.forceSleepWake();
  // delay(1);
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(_ssid, _password);
}

bool GatewayClient::connect()
{

  for (int i = 0; i < MAX_CONNECT_RETRY; i++)
  {

    if (WiFi.status() == WL_CONNECTED)
    {
      WiFi.localIP().toString().toCharArray(ip, 16);
      return true;
    }

    delay(500);
  }

  strcpy(ip, "-");

  return false;
}

WiFiClient *GatewayClient::openMsgPackStream(const char *url)
{
  _http.useHTTP10(true); // see https://arduinojson.org/v6/how-to/use-arduinojson-with-esp8266httpclient/
  _http.begin(url);
  _http.addHeader(HEADER_ACCEPT, CONTENT_TYPE_MSG_PACK);
  if (_timeStamp[0])
    _http.addHeader(HEADER_IF_MODIFIED_SINCE, _timeStamp);

  const char *responseHeaders[] = {"Date", "Last-Modified"};
  _http.collectHeaders(responseHeaders, sizeof(responseHeaders) / sizeof(char *));

  int httpCode = _http.GET();

  if (httpCode == 200)
  {
    String dateHeader = _http.header("Date");
    dateHeader.toCharArray(_timeStamp, HTTP_TIME_STAMP_LENGTH);

    return _http.getStreamPtr();
  }
  else
  {
    // it could be error, or just 304 (NOT MODIFIED) as a response to Last-Modified header
    return nullptr;
  }

  // parse State Machine Definition provided as MsgPack Stream
  //  DeserializationError error = deserializeMsgPack(
  //      stateMachineDefinition,
  //      _http.getStream(),
  //      DeserializationOption::NestingLimit(JSON_NESTING_LIMIT));
}

void GatewayClient::closeMsgPackStream()
{
  _http.end();
}