#include <Arduino.h>

#include "HubClient.h"

HubClient::HubClient() : _localTimeHandler()
{
  timeStamp[0] = 0;
}

void HubClient::init(const char *ssid, const char *password)
{
  _ssid = ssid;
  _password = password;
}

void HubClient::off()
{
  WiFi.mode(WIFI_OFF);
  // WiFi.forceSleepBegin();
  delay(1);
}

void HubClient::on()
{
  // WiFi.forceSleepWake();
  // delay(1);
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(_ssid, _password);
}

bool HubClient::isConnected()
{
  return WiFi.status() == WL_CONNECTED;
}

bool HubClient::connect()
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

bool HubClient::ensureConnection()
{
  if (isConnected())
    return true;

  Serial.println(F("Reconnecting Wifi"));
  off();
  on();
  if (connect())
    return true;

  Serial.println(F("Wifi connection failed..."));
  return false;
}

void HubClient::postMsgPack(const char *url, const uint8_t *payload, size_t size)
{
  if (!ensureConnection())
    return;

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

WiFiClient *HubClient::openMsgPackStream(const char *url, const char *ifModifiedSince)
{
  if (!ensureConnection())
    return nullptr;

  _http.useHTTP10(true); // see https://arduinojson.org/v6/how-to/use-arduinojson-with-esp8266httpclient/
  _http.begin(url);
  _http.addHeader(HEADER_ACCEPT, CONTENT_TYPE_MSG_PACK);
  if (ifModifiedSince && ifModifiedSince[0])
    _http.addHeader(HEADER_IF_MODIFIED_SINCE, ifModifiedSince);

  const char *responseHeaders[] = {"Date", "Last-Modified"};
  _http.collectHeaders(responseHeaders, sizeof(responseHeaders) / sizeof(char *));

  int httpCode = _http.GET();

  String dateHeader = _http.header("Date");
  _localTimeHandler.update(dateHeader);
  dateHeader.toCharArray(timeStamp, HTTP_TIME_STAMP_LENGTH);

  if (httpCode == 200)
  {

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
}

void HubClient::closeMsgPackStream()
{
  _http.end();
}
