#ifndef wificlient_h
#define wificlient_h

#define MAX_CONNECT_RETRY 10

const char HEADER_ACCEPT[] = "accept";
const char HEADER_IF_MODIFIED_SINCE[] = "if-modified-since";

const char CONTENT_TYPE_MSG_PACK[] = "application/msgpack";

class WiFiClient
{
public:
  WiFiClient();
  void init(char *ssid, char *password);
  bool connect();
  void off();
  void on();

  void getMsgPack(const char *url);

  char ip[16];

private:
  const char *_ssid;
  const char *_password;

  char _timeStamp[32];
};

#endif
