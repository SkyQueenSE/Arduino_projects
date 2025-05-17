#include <IPAddress.h>

#ifndef STASSID
#define STASSID "YOUR_SSID"
#define STAPSK "YOUR_SSID_PASSWORD"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;
const char *host = "LOCAL_IP_OF_ESP8266";

const char *www_username = "LOGIN_CREDENTIALS_USERNAME";
const char *www_password = "LOGIN_CREDENTIALS_PASSWORD";

//Stuff for updating dynamic IP by a GET request, see https://support.loopia.se/wiki/om-dyndns-stodet/
//Credentials to send request to Loopia
String base64 = "abcdefgh1235678jdhklrl1233o1abcd";
String authHeader = "Basic " + base64;

String loopia = "http://dyndns.loopia.se/?hostname=";

String myDomains[] = {"DOMAIN1.COM", "DOMAIN2.COM"};

// The local IP of the reverse proxy, which will not be accepted for local requests
IPAddress reverseProxyIP = IPAddress(192, 168, 1, 99);
