#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

#include "WiFiClient.h"

WiFiClient::WiFiClient() {}

void WiFiClient::init(char *ssid, char *password)
{
  _ssid = ssid;
  _password = password;
  _timeStamp[0] = 0;
}

void WiFiClient::off()
{
  WiFi.mode(WIFI_OFF);
  WiFi.forceSleepBegin();
  delay(1);
}

void WiFiClient::on()
{
  WiFi.forceSleepWake();
  delay(1);
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(_ssid, _password);
}

bool WiFiClient::connect()
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

void WiFiClient::getMsgPack(const char *url)
{
  HTTPClient http;
  http.begin(url);
  http.addHeader(HEADER_ACCEPT, CONTENT_TYPE_MSG_PACK);
  if (_timeStamp[0])
    http.addHeader(HEADER_IF_MODIFIED_SINCE, _timeStamp);
}