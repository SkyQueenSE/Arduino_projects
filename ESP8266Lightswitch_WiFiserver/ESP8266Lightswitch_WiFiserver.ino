#include "secrets.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <TelnetStream.h>

ESP8266WebServer server(80);

const long utcOffsetInSeconds = 7200;
char daysOfTheWeek[7][12] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
String timeString = "hello";

const float latitude = 59.6099;
const float longitude = 16.5448;
int sunriseHour;
int sunriseMinute;
int sunsetHour;
int sunsetMinute;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

unsigned long currentMillis;
unsigned long previousNTPMillis = 0;
long NTPInterval = 600000;

unsigned long previousSunMillis = 0;
long sunInterval = 6000000;

const int switchPin = 0;
bool lightStatus = false;

WiFiClient client;
HTTPClient http;

void setup() {

  pinMode(switchPin, OUTPUT);
  digitalWrite(switchPin, LOW);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    delay(1000);
    ESP.restart();
  }
  ArduinoOTA.begin();
  TelnetStream.begin();

  timeClient.begin();
  
  updateTime();  
  sunsetSunriseTimeUpdate();

  server.on("/", handleRoot);
  server.on("/switch", HTTP_GET, switchRelay);
  server.on("/status", HTTP_GET, returnStatus);

  server.collectHeaders("Cookie");

  server.begin();
}

void handleRoot() {

  String bgColor;
  String buttonColor;

  switch (lightStatus) {
    case true:
      bgColor = "#ffa129";
      buttonColor = "#1d0f95";
      break;
    case false:
      bgColor = "#1d0f95";
      buttonColor = "#ffa129";
      break;
  }

  String message = "<html><head><title>Switch Control</title><style>body { font-size: 40px; }"
                   "button, input[type='submit'], input[type='text'], input[type='password'] { font-size: 40px; padding: 20px; margin: 10px; }"
                   ".time-string { font-size: 80px; line-height: 1.2; }"
                   "</style></head><body style='background-color:"
                   + bgColor + ";'>"
                               "<h1>Switch Control</h1>"
                               "<form action=\"/switch\" method=\"GET\">"
                               "<input type=\"submit\" value=\"PUSH MEEEE\" style=\"background-color:"
                   + buttonColor + "; color: black; border: none;\"/>"
                                   "</form>"
                                   "<div class=time-string>"
                   + timeString + "</div>"
                                  "<br><br>"
                                  "</body></html>";
  server.send(200, "text/html", message);
}

void switchRelay() {
  if (lightStatus == true) {
    digitalWrite(switchPin, LOW);
    lightStatus = false;
  } else {
    digitalWrite(switchPin, HIGH);
    lightStatus = true;
  }

  // Redirect back to the root page or provide a response indicating the action was successful
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "Switched relay; Redirecting back...");
}

void returnStatus() {
  if (lightStatus == true) {
    server.send(200, "text/plain", "on");
  }
  else {
    server.send(200, "text/plain", "off");
  }
}

void timedLightUpdate() {
  if (timeClient.getHours() == 20 && timeClient.getMinutes() == 00 && timeClient.getSeconds() < 5) {
    digitalWrite(switchPin, HIGH);
    lightStatus = true;
  } else if (timeClient.getHours() == 7 && timeClient.getMinutes() == 00 && timeClient.getSeconds() < 5) {
    digitalWrite(switchPin, LOW);
    lightStatus = false;
  }
}

void updateTime() {
  timeClient.update();
}


float calculateSun(bool sunrise) {
  time_t rawTime = timeClient.getEpochTime();
  struct tm *timeInfo = gmtime(&rawTime);
  int dayOfYear = timeInfo->tm_yday + 1;
  float solarDeclination = 23.45 * sin(2 * PI * (284 + dayOfYear) / 365);
  float B = 2 * PI * (dayOfYear - 81) / 365;
  float equationOfTime = 9.87 * sin(2 * B) - 7.53 * cos(B) - 1.5 * sin(B);
  float solarNoon = 12.0 + (4.0 * longitude / 60.0) - equationOfTime / 60.0;
  float hourAngle = acos(-tan(latitude * PI / 180) * tan(solarDeclination * PI / 180)) * 180 / PI;

  if (sunrise)
    return solarNoon - hourAngle / 15.0;
  else
    return solarNoon + hourAngle / 15.0;
}

void sunsetSunriseTimeUpdate() {
  float sunrise = calculateSun(true);
  float sunset = calculateSun(false);

  sunriseHour = int(sunrise);
  sunriseMinute = int((sunrise - sunriseHour) * 60) - 30;
  sunsetHour = int(sunset);
  sunsetMinute = int((sunset - sunsetHour) * 60);

  if (sunriseMinute < 0) {
    sunriseMinute += 60;
    sunriseHour -= 1;

    if (sunriseHour < 0) {
      sunriseHour = 23;
    }
  }
}

void switchLightSunsetSunlight() {
  if (timeClient.getHours() == (sunriseHour) && timeClient.getMinutes() == (sunriseMinute) && timeClient.getSeconds() < 5) {
    digitalWrite(switchPin, LOW);
    lightStatus = false;
  } else if (timeClient.getHours() == sunsetHour && timeClient.getMinutes() == sunsetMinute && timeClient.getSeconds() < 5) {
    digitalWrite(switchPin, HIGH);
    lightStatus = true;
  }
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();

  switchLightSunsetSunlight();

  currentMillis = millis();
  if (currentMillis - previousNTPMillis > NTPInterval) {
    previousNTPMillis = currentMillis;
    updateTime();
  }
  timeString = timeClient.getFormattedTime();

  if (currentMillis - previousSunMillis > sunInterval) {
    previousSunMillis = currentMillis;
    sunsetSunriseTimeUpdate();
  }
}
