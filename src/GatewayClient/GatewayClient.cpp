#include <Arduino.h>

#include "GatewayClient.h"

GatewayClient::GatewayClient()
{
  timeStamp[0] = 0;
}

void GatewayClient::init(const char *ssid, const char *password)
{
  _ssid = ssid;
  _password = password;
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

bool GatewayClient::isConnected()
{
  return WiFi.status() == WL_CONNECTED;
}

bool GatewayClient::connect()
{

  for (int i = 0; i < MAX_CONNECT_RETRY; i++)
  {

    if (isConnected())
    {
      WiFi.localIP().toString().toCharArray(ip, 16);
      return true;
    }

    delay(1000);
    Serial.println(".");
  }

  strcpy(ip, "-");

  return false;
}

void GatewayClient::postMsgPack(const char *url, const uint8_t *payload, size_t size)
{
  Serial.print(F("Posting data to: "));
  Serial.println(url);

  _http.begin(url);
  _http.addHeader(HEADER_CONTENT_TYPE, CONTENT_TYPE_MSG_PACK);
  int httpCode = _http.POST((uint8_t *)payload, size);
  if (httpCode != 201)
  {
    Serial.print(F("Failed posting:"));
    Serial.println(httpCode);
  }
  _http.end();
}

WiFiClient *GatewayClient::openMsgPackStream(const char *url, bool validateUpdateTime)
{
  _http.useHTTP10(true); // see https://arduinojson.org/v6/how-to/use-arduinojson-with-esp8266httpclient/
  _http.begin(url);
  _http.addHeader(HEADER_ACCEPT, CONTENT_TYPE_MSG_PACK);
  if (validateUpdateTime && timeStamp[0])
    _http.addHeader(HEADER_IF_MODIFIED_SINCE, timeStamp);

  const char *responseHeaders[] = {"Date", "Last-Modified"};
  _http.collectHeaders(responseHeaders, sizeof(responseHeaders) / sizeof(char *));

  int httpCode = _http.GET();

  if (httpCode == 200)
  {
    if (validateUpdateTime)
    {
      String dateHeader = _http.header("Date");
      dateHeader.toCharArray(timeStamp, HTTP_TIME_STAMP_LENGTH);
    }

    return _http.getStreamPtr();
  }
  else if (httpCode == 304)
  {
    // 304 (NOT MODIFIED) as a response to Last-Modified header
    Serial.println(F("Definition up to date"));
    return nullptr;
  }
  else
  {
    // any error
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
