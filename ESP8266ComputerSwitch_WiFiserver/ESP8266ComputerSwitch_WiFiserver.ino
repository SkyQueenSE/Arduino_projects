#include "secrets.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <TelnetStream.h>
#include <IPAddress.h>

#include <map>

std::map<String, String> sessionMap;

ESP8266WebServer server(80);

const int onPin = 0;
const int switchPin = 2;

const long interval = 60000;
unsigned long previousMillis = 0;  // Calculate Elapsed Time & Trigger
unsigned long currentMillis = 0;   // Calculate Elapsed Time & Trigger

bool computerStatus = true;
long checkStatusInterval = 2000;
const int samples = 10;
unsigned long lastCheckMillis = 0;
enum ComputerState { COMPUTER_OFF,
                     COMPUTER_ON,
                     COMPUTER_ASLEEP };
ComputerState computerState;

//For cookie
String currentSessionId = "";

String newIp = "2";
String oldIp = "1";
String asciiArt;

WiFiClient client;
HTTPClient http;

void setup() {
  TelnetStream.begin(115200);

  pinMode(onPin, INPUT);
  pinMode(switchPin, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    TelnetStream.println("WiFi Connect Failed! Rebooting...");
    delay(1000);
    ESP.restart();
  }
  ArduinoOTA.begin();
  TelnetStream.begin();

  server.on("/", handleRoot);
  server.on("/switch", HTTP_GET, switchRelay);
  server.on("/off", HTTP_GET, turnOff);
  server.on("/localswitch", HTTP_GET, localSwitchRequest);
  server.on("/localoff", HTTP_GET, localOffRequest);
  server.on("/getstatus", HTTP_GET, getStatus);
  server.on("/login", handleLogin);

  server.collectHeaders("Cookie");

  server.begin();

  TelnetStream.println("Starting...");
}

bool is_authenticated() {
  if (server.hasHeader("Cookie")) {
    String cookie = server.header("Cookie");
    String clientIP = server.client().remoteIP().toString();
  if (sessionMap.count(clientIP) && cookie.indexOf("ESPSESSIONID=" + sessionMap[clientIP]) != -1) {
      TelnetStream.println("Authentication Successful");
      return true;
    }
  }
  TelnetStream.println("Authentication Failed");
  return false;
}


void handleLogin() {
  String msg;
  if (server.hasHeader("Cookie")) {
    TelnetStream.print("Found cookie: ");
    String cookie = server.header("Cookie");
    TelnetStream.println(cookie);
  }
  if (server.hasArg("DISCONNECT")) {
    TelnetStream.println("Disconnection");
    String clientIP = server.client().remoteIP().toString();
    sessionMap.erase(clientIP);
    currentSessionId = "";
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.sendHeader("Set-Cookie", "ESPSESSIONID=0; Max-Age=0; HttpOnly");
    server.send(301);
    return;
  }
  if (server.hasArg("USERNAME") && server.hasArg("PASSWORD")) {
    if (server.arg("USERNAME") == www_username && server.arg("PASSWORD") == www_password) {
      server.sendHeader("Location", "/");
      server.sendHeader("Cache-Control", "no-cache");
      String clientIP = server.client().remoteIP().toString();
      currentSessionId = String(random(100000, 999999));
      sessionMap[clientIP] = currentSessionId;
      server.sendHeader("Set-Cookie", "ESPSESSIONID=" + currentSessionId + "; HttpOnly");
      server.send(301);
      TelnetStream.println("Log in Successful");
      return;
    }
    msg = "Wrong username/password! try again.";
    TelnetStream.println("Log in Failed");
    server.sendHeader("Set-Cookie", "ESPSESSIONID=0; Max-Age=0; HttpOnly");
  }

  String content = "<html><body><form action='/login' method='POST'>To log in, please enter credentials:<br>"
                   "User:<input type='text' name='USERNAME' placeholder='user name'><br>"
                   "Password:<input type='password' name='PASSWORD' placeholder='password'><br>"
                   "<input type='submit' name='SUBMIT' value='Submit'></form>"
                   + msg + "<br>"
                           "&nbsp;&nbsp;&nbsp;&#x2571;&verbar;&#x3001;"
                           "<br>"
                           "&nbsp;&lpar;&ring;&#x2ce;&nbsp;&#x3002;&#x37;&#x20;&#x20;"
                           "<br>"
                           "&nbsp;&nbsp;&verbar;&#x3001;&tilde;&#x3035;&#x20;&#x20;&#x20;&#x20;&#x20;&#x20;&#x20;&#x20;&#x20;&#x20;"
                           "<br>"
                           "&nbsp;&#x3058;&#x3057;&#x2cd;&comma;&rpar;&#x30ce";
  server.send(200, "text/html", content);
}

void handleRoot() {

  if (!is_authenticated()) {
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
    return;
  }
  // Determine the background color based on the computer state
  String bgColor;
  switch (computerState) {
    case COMPUTER_ON:
      bgColor = "#37ff37";  // Green background for ON state
      asciiArt = "&nbsp;&nbsp;&nbsp;&#x2571;&verbar;&#x3001;"
                 "<br>"
                 "&nbsp;&lpar;&ring;&#x2ce;&nbsp;&#x3002;&#x37;&#x20;&#x20;"
                 "<br>"
                 "&nbsp;&nbsp;&verbar;&#x3001;&tilde;&#x3035;&#x20;&#x20;&#x20;&#x20;&#x20;&#x20;&#x20;&#x20;&#x20;&#x20;"
                 "<br>"
                 "&nbsp;&#x3058;&#x3057;&#x2cd;&comma;&rpar;&#x30ce";
      break;
    case COMPUTER_OFF:
      bgColor = "#737373";  // Gray background for OFF state
      break;
    case COMPUTER_ASLEEP:
      bgColor = "#5dd7ff";  // Blue background for ASLEEP state
      asciiArt = "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&verbar;&bsol;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&lowbar;&comma;&comma;&comma;&#x2d;&#x2d;&#x2d;&comma;&comma;&lowbar;"
                 "<br>"
                 "&#x5a;&#x5a;&#x5a;&#x7a;&#x7a;&nbsp;&sol;&comma;&grave;&period;&#x2d;&apos;&grave;&apos;&nbsp;&nbsp;&nbsp;&nbsp;&#x2d;&period;&nbsp;&nbsp;&semi;&#x2d;&semi;&semi;&comma;&lowbar;"
                 "<br>"
                 "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&verbar;&comma;&#x34;&#x2d;&nbsp;&nbsp;&rpar;&nbsp;&rpar;&#x2d;&comma;&lowbar;&period;&nbsp;&comma;&bsol;&nbsp;&lpar;&nbsp;&nbsp;&grave;&apos;&#x2d;&apos;"
                 "<br>"
                 "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&apos;&#x2d;&#x2d;&#x2d;&apos;&apos;&lpar;&lowbar;&sol;&#x2d;&#x2d;&apos;&nbsp;&nbsp;&grave;&#x2d;&apos;&bsol;&lowbar;&rpar;";
      break;
    default:
      bgColor = "white";  // Default background color
  }


  String message = "<html><head><title>Switch Control</title><style>body { font-size: 40px; }"
                   "button, input[type='submit'], input[type='text'], input[type='password'] { font-size: 40px; padding: 20px; margin: 10px; }"
                   ".ascii-art { font-size: 80px; line-height: 1.2; }"
                   "</style></head><body style='background-color:"
                   + bgColor + ";'>"
                               "<h1>Switch Control</h1>"
                               "<form action=\"/switch\" method=\"GET\">"
                               "<input type=\"submit\" value=\"PUSH MEEEE\"/>"
                               "</form>"
                               "<div class=\"ascii-art\">"
                   + asciiArt + "</div>"
                                "<br><br>"
                                "<form action=\"/off\" method=\"GET\">"
                                "<input type=\"submit\" value=\"Turn off\"/>"
                                "</form>"
                                "<br><br>"
                                "<a href=\"/login?DISCONNECT=YES\">Logout</a>"
                                "</body></html>";
  server.send(200, "text/html", message);
}

void switchRelay() {

  if (!is_authenticated()) {
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
    return;
  }
  digitalWrite(switchPin, HIGH);
  delay(200);
  digitalWrite(switchPin, LOW);

  checkStatusInterval = 1000;

  // Redirect back to the root page or provide a response indicating the action was successful
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "Switched relay for 200ms; Redirecting back...");
}

void turnOff() {
  if (!is_authenticated()) {
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
    return;
  }
  digitalWrite(switchPin, HIGH);
  delay(5000);
  digitalWrite(switchPin, LOW);

  checkStatusInterval = 1000;

  // Redirect back to the root page or provide a response indicating the action was successful
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "Switched relay for 1000ms; Redirecting back...");
}

void localSwitchRequest() {
  IPAddress clientIP = server.client().remoteIP();
  TelnetStream.print("Request from IP: ");
  TelnetStream.println(clientIP);
  if (clientIP != reverseProxyIP && clientIP[0] == 10 && clientIP[1] == 0 && clientIP[2] == 1) {
    TelnetStream.println("Local switching!");
    digitalWrite(switchPin, HIGH);
    delay(200);
    digitalWrite(switchPin, LOW);

    checkStatusInterval = 1000;

    // Redirect back to the root page or provide a response indicating the action was successful
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "Switched relay for 200ms; Redirecting back...");
  } else {
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
    return;
  }
}

void localOffRequest() {
  IPAddress clientIP = server.client().remoteIP();
  TelnetStream.print("Request from IP: ");
  TelnetStream.println(clientIP);
  if (clientIP != reverseProxyIP && clientIP[0] == 10 && clientIP[1] == 0 && clientIP[2] == 1) {
    TelnetStream.println("Local switching!");
    digitalWrite(switchPin, HIGH);
    delay(5000);
    digitalWrite(switchPin, LOW);

    checkStatusInterval = 1000;

    // Redirect back to the root page or provide a response indicating the action was successful
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "Switched relay for 200ms; Redirecting back...");
  } else {
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
    return;
  }
}

String getIP() {
  // ######## GET PUBLIC IP ######## //
  String myIp = "";
  http.begin(client, "http://ifconfig.me/ip");
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK) {
    myIp = http.getString();
    TelnetStream.println("I got the IP: " + myIp);
  }
  http.end();
  return myIp;
}

void updateDDNS(String myIp) {
  if (WiFi.status() == WL_CONNECTED) {
    // Bygg URL för DDNS-uppdatering för alla domäner
    for (int i = 0; i < (sizeof(myDomains) / sizeof(myDomains[0])); i++) {

      String url = loopia + myDomains[i] + "&myip=" + myIp;
      http.begin(client, url);                      // Börja HTTP-förfrågan
      http.addHeader("Authorization", authHeader);  // Lägg till Base64-kodade autentiseringsuppgifter i headern

      int httpCode = http.GET();  // Skicka GET-förfrågan

      if (httpCode > 0) {  //Kontrollera för svar från servern
        String payload = http.getString();
        TelnetStream.print("Payload: ");
        TelnetStream.println(payload);
        oldIp = myIp;
      } else {
        TelnetStream.println("Error on sending GET request for " + myDomains[i]);
      }
    }

    http.end();  //Avsluta anslutningen
  }
}

void checkComputerStatus() {

  unsigned long currentTime = millis();
  int currentState = 0;
  int oldState = computerState;

  if (currentTime - lastCheckMillis >= checkStatusInterval) {
    lastCheckMillis = currentTime;
    for (int i = 0; i < 20; ++i) {
      currentState += digitalRead(onPin);
      delay(25);
    }

    if (currentState == 20) {
      computerState = COMPUTER_ON;
    } else if (currentState == 0) {
      computerState = COMPUTER_OFF;
    } else {
      computerState = COMPUTER_ASLEEP;
    }
    if (oldState != computerState) {
      checkStatusInterval = 20000;
    }
  }
}

void getStatus() {
  String message = "null";
  if (computerState == COMPUTER_OFF) {
    message = "off";
  } else if (computerState == COMPUTER_ON) {
    message = "on";
  } else if (computerState == COMPUTER_ASLEEP) {
    message = "sleep";
  }
  server.send(200, "text/plain", message);
  return;
}


void loop() {
  ArduinoOTA.handle();
  server.handleClient();

  checkComputerStatus();

  currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    newIp = getIP();
    if (oldIp != newIp) {
      TelnetStream.println("Old IP is not New IP. Old: " + oldIp + "New: " + newIp);
      updateDDNS(newIp);
    }
  }
}
