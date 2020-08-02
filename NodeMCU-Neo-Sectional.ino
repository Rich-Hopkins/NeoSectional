#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <ezTime.h>
#include "arduino_secrets.h"

#define PIN 5 // the pin your strip is connected to 
#define COUNT 49 // how many led's are on that strip  

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(COUNT, PIN, NEO_GRB + NEO_KHZ800);
uint32_t Color_IFR = pixels.Color(0, 255, 0);
uint32_t Color_VFR = pixels.Color(255, 0, 0);
uint32_t Color_MVFR = pixels.Color(0, 0, 255);
uint32_t Color_LIFR = pixels.Color(0, 125, 200);
uint32_t Color_Clear = pixels.Color(0, 0, 0);

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
char server[] = "http://aerie.starfireaviation.com/weather/metars/atlanta?data=flight_category&data=observed";

ESP8266WiFiMulti WiFiMulti;

void setup() {

  Serial.begin(500000);
  while (!Serial) {
    ;
  }
  Serial.println(F("\nStarting Sketch"));
  digitalWrite(LED_BUILTIN, LOW);
  pixels.begin();
  pixels.clear();
  pixels.setBrightness(50);
  pixels.setPixelColor(45, Color_IFR);
  pixels.setPixelColor(46, Color_VFR);
  pixels.setPixelColor(47, Color_MVFR);
  pixels.setPixelColor(48, Color_LIFR);
  pixels.show();
  Serial.println(F("Lights initialized"));
  connectToWifi();

  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.println(F("Connecting to wifi..."));
    delay(1000);
  }
  Serial.println(F("Wifi connected"));
  waitForSync();
}

void loop() {
  //wait for WiFi connection
  if (WiFiMulti.run() != WL_CONNECTED) {
    connectToWifi();
    delay(20000);
    return;
  }
  else {
    WiFiClient client;
    HTTPClient http;
    if (http.begin(client, server)) {  // HTTP


      Serial.println(F("[HTTP] GET..."));
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();
          Serial.println(payload);

          StaticJsonDocument<2500> doc;
          auto error = deserializeJson(doc, payload);
          if (error) {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(error.c_str());
          } else {
            String currentUTCTime = UTC.dateTime(ISO8601);
            time_t currentTime = now();

            for (JsonObject obj : doc.as<JsonArray>()) {
              Serial.println(F("\nCurrent Time: "));
              Serial.println(currentUTCTime);
              Serial.println(currentTime);
              String icao = obj["icao"];
              String flight_category = obj["flight_category"];
              String observed = obj["observed"];
              unsigned long timeDifference  = timeDiff(observed, currentTime);
              Serial.print(F("Difference: "));
              Serial.println(timeDifference);
              setLight(icao, flight_category, observed, timeDifference);
            }
          }
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        return;
      }

      http.end();
    } else {
      Serial.printf("[HTTP} Unable to connect\n");
      return;
    }
  }

  delay(590000);
  waitForSync();
  updateNTP();
  delay(10000);
}

void connectToWifi() {
  Serial.println(F("Connecting..."));
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, pass);
}

unsigned long timeDiff(String observed, time_t current) {
  int year = observed.substring(0, 4).toInt();
  int month = observed.substring(5, 7).toInt();
  int day = observed.substring(8, 10).toInt();
  int hour = observed.substring(11, 13).toInt();
  int minute = observed.substring(14, 16).toInt();
  int second = observed.substring(17, 19).toInt();
  time_t observedTime = makeTime(hour, minute, second, day, month, year);
  return current - observedTime;
}

void setLight(String icao, String flight_category, String observed, unsigned long timeDifference) {
  int pixel;
  if (icao == "KATL") {
    pixel = 20;
  }
  else if (icao == "KCCO") {
    pixel = 16;
  }
  else if (icao == "KCNI") {
    pixel = 0;
  }
  else if (icao == "KCTJ") {
    pixel = 8;
  }
  else if (icao == "KCVC") {
    pixel = 37;
  }
  else if (icao == "KFFC") {
    pixel = 18;
  }
  else if (icao == "KFTY") {
    pixel = 21;
  }
  else if (icao == "KGVL") {
    pixel = 31;
  }
  else if (icao == "KHMP") {
    pixel = 19;
  }
  else if (icao == "KLGC") {
    pixel = 13;
  }
  else if (icao == "KLZU") {
    pixel = 29;
  }
  else if (icao == "KMGE") {
    pixel = 22;
  }
  else if (icao == "KPDK") {
    pixel = 26;
  }
  else if (icao == "KPUJ") {
    pixel = 5;
  }
  else if (icao == "KRYY") {
    pixel = 23;
  }
  else if (icao == "KVPC") {
    pixel = 3;
  }
  else if (icao == "KWDR") {
    pixel = 34;
  }

  if (timeDifference > 7200) {
    pixels.setPixelColor(pixel, Color_Clear);
    Serial.print(F("Stale data :"));
    Serial.println(observed);
  }
  else if (flight_category == "IFR") {
    pixels.setPixelColor(pixel, Color_IFR);
  }
  else if (flight_category == "VFR") {
    pixels.setPixelColor(pixel, Color_VFR);
  }
  else if (flight_category == "MVFR") {
    pixels.setPixelColor(pixel, Color_MVFR);
  }
  else if (flight_category == "LIFR") {
    pixels.setPixelColor(pixel, Color_LIFR);
  }

  Serial.print(F("ICAO: "));
  Serial.println(icao);
  Serial.print(F("Condition: "));
  Serial.println(flight_category);
  Serial.print(F("Observed: "));
  Serial.println(observed);
  pixels.show();
}
