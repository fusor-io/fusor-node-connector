#include <Arduino.h>

#include "../PrintWrapper/PrintWrapper.h"
#include "HubClient.h"

HubClient::HubClient() : _localTimeHandler()
{
  timeStamp[0] = 0;
}

void HubClient::init(const char *ssid, const char *password, const char *staticIp, const char *gateway, const char *subnet)
{
  _ssid = ssid;
  _password = password;
  _staticIp = staticIp;
  _gateway = gateway;
  _subnet = subnet;
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
  if (strlen(_staticIp))
  {
    IPAddress staticIP;
    IPAddress gateway;
    IPAddress subnet;

    if (staticIP.fromString(_staticIp) && gateway.fromString(_gateway) && subnet.fromString(_subnet))
    {
      WiFi.config(staticIP, gateway, subnet);
    }
  }
  _wifiMulti.addAP(_ssid, _password);
  // WiFi.begin(_ssid, _password);
}

bool HubClient::isConnected()
{
  // return WiFi.status() == WL_CONNECTED;
  return _wifiMulti.run(MAX_CONNECT_TIMEOUT) == WL_CONNECTED;
}

void HubClient::listNetworks()
{
  // WiFi.scanNetworks will return the number of networks found
  Serial << F("Scanning networks\n");
  int n = WiFi.scanNetworks();

  if (n == 0)
  {
    Serial << F("No networks found\n");
  }
  else
  {
    Serial << n << F(" networks found\n");
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial << i + 1 << ": " << WiFi.SSID(i) << " (" << WiFi.RSSI(i) << ")\n";
      delay(10);
    }
  }
}

bool HubClient::connect()
{

  for (int i = 0; i < MAX_CONNECT_RETRY; i++)
  {

    if (isConnected())
    {
      WiFi.localIP().toString().toCharArray(ip, 16);
      Serial << F("MAC Address: ") << WiFi.macAddress() << "\n";
      return true;
    }

    delay(1000);

    Serial << ".\n";
  }

  Serial << "'" << _ssid << "' (" << _password << ")\n";

  Serial << F("MAC Address: ") << WiFi.macAddress() << "\n";

  listNetworks();

  Serial << "Wifi status: ";
  switch (_wifiMulti.run())
  {
  case WL_NO_SSID_AVAIL:
    Serial << F("WL_NO_SSID_AVAIL\n");
    break;
  case WL_CONNECT_FAILED:
    Serial << F("WL_CONNECT_FAILED\n");
    // Turn radio OFF and delete AP config from NVS memory
    Serial << F("Resetting WiFi\n");
    WiFi.disconnect(true, true);
    break;
  case WL_IDLE_STATUS:
    Serial << F("WL_IDLE_STATUS\n");
    break;
  case WL_CONNECTION_LOST:
    Serial << F("WL_CONNECTION_LOST\n");
    break;
  case WL_DISCONNECTED:
    Serial << F("WL_DISCONNECTED\n");
    break;
  default:
    Serial << _wifiMulti.run() << "\n";
  }

  strcpy(ip, "-");

  return false;
}

bool HubClient::ensureConnection()
{
  if (isConnected())
    return true;

  Serial << F("Reconnecting Wifi\n");
  off();
  on();
  if (connect())
    return true;

  Serial << F("Wifi connection failed...\n");
  return false;
}

void HubClient::postMsgPack(const char *url, const uint8_t *payload, size_t size)
{
  if (!ensureConnection())
    return;

  // dont use F() to reduce latency
  Serial << "Posting data to: " << url << "\n";

  _http.begin(_client, url);
  _http.addHeader(HEADER_CONTENT_TYPE, CONTENT_TYPE_MSG_PACK);
  int httpCode = _http.POST((uint8_t *)payload, size);
  if (httpCode != 201)
    Serial << F("Failed posting:") << httpCode << "\n";

  _http.end();
}

WiFiClient *HubClient::openMsgPackStream(const char *url, const char *ifModifiedSince)
{
  if (!ensureConnection())
    return nullptr;

  _http.useHTTP10(true); // see https://arduinojson.org/v6/how-to/use-arduinojson-with-esp8266httpclient/
  _http.begin(_client, url);
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
    // do not use F() to reduce latency
    Serial << "Definition up to date\n";
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
