#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
//#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <ezTime.h>

#define PIN 5     // D1 the pin your strip is connected to 
#define COUNT 49  // how many led's are on that strip  
#define WIFI_DELAY 500 // Delay paramter for connection
#define MAX_SSID_LEN 32 //Max SSID octets
#define MAX_CONNECT_TIME 30000 //Wait this much until device gets IP

char ssid[MAX_SSID_LEN] = "";  //SSID that to be stored to connect
char secureSSID[][MAX_SSID_LEN] = {
  "RichPixel"
  , "FBIMobileTaskForce3"
};
char securePass[][11] = {
  "7706586798"
  , "7706586798"
};
const int NIGHT = 16; // D2 This pin will be used for the on/off for nighttime
char server[] = "http://aerie.starfireaviation.com/weather/metars/atlanta?data=flight_category&data=observed";

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(COUNT, PIN, NEO_GRB + NEO_KHZ800);
uint32_t Color_IFR = pixels.Color(0, 255, 0);
uint32_t Color_VFR = pixels.Color(255, 0, 0);
uint32_t Color_MVFR = pixels.Color(0, 0, 255);
uint32_t Color_LIFR = pixels.Color(0, 125, 200);
uint32_t Color_Clear = pixels.Color(0, 0, 0);

Timezone est;

void setup() {
  pinMode(NIGHT, INPUT);
  Serial.begin(500000);
  while (!Serial) {
    ;
  }
  Serial.println(F("\nStarting Sketch"));
  pixels.clear();
  pixels.show();
  initializeLights();
  while (WiFi.status() != WL_CONNECTED) {
    connectToWifi();
  }
  Serial.println(F("\nWifi connected"));
}

void loop() {
  //wait for WiFi connection
  while (WiFi.status() != WL_CONNECTED) {
    connectToWifi();
    delay(30000);
    return;
  }
  Serial.print(F("Syncing time."));
  while (!waitForSync()) {
    Serial.print(F("."));
    delay(1000);
  }
  Serial.println();
  updateNTP();
  delay(15000);
  est.setLocation(F("America/New_York"));

  WiFiClient client;
  HTTPClient http;
  if (http.begin(client, server)) {  // HTTP
    time_t currentTime = now();
    Serial.println("New York Time:          " + est.dateTime(ISO8601));
    Serial.println("Current UTC Time:     " + UTC.dateTime(ISO8601));
    uint8_t hr = hour(est.now());
    Serial.print("New York hour: " );
    Serial.println(hr);
    Serial.print (F("Night Mode: "));
    Serial.println(digitalRead(NIGHT));
    if ((hr > 21 || hr < 6) && !digitalRead(NIGHT)) {
      Serial.println(F("Night mode is off and it is nighttime."));
      pixels.clear();
      pixels.show();
    }

    else {
      Serial.println(F("Night mode is on or it is daytime."));
      Serial.println(F("[HTTP] GET..."));
      // start connection and send HTTP header
      int httpCode = http.GET();

      // httpCode will be negative on error
      if (httpCode > 0) {
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK) { // || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = http.getString();
          //Serial.println(payload);

          StaticJsonDocument<2500> doc;
          auto error = deserializeJson(doc, payload);
          if (error) {
            Serial.print(F("deserializeJson() failed with code "));
            Serial.println(error.c_str());
          } else {
            initializeLights();
            for (JsonObject obj : doc.as<JsonArray>()) {
              String icao = obj["icao"];
              String flight_category = obj["flight_category"];
              String observed = obj["observed"];
              unsigned long timeDifference  = timeDiff(observed, currentTime);
              setLight(icao, flight_category, observed, timeDifference);
            }
          }
        }
      }
      else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        return;
      }
    }

    http.end();
  } else {
    Serial.printf("[HTTP} Unable to connect\n");
    return;
  }
  delay(600000); //10 minutes
  Serial.println(F("End of loop\n\n"));
}

void connectToWifi() {
  /* Clear previous modes. */
  WiFi.softAPdisconnect();
  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  delay(WIFI_DELAY);
  /* Scan for networks to find open guy. */
  scanAndSort();
  delay(WIFI_DELAY);
  /* Global ssid param need to be filled to connect. */
  if (strlen(ssid) > 0) {
    Serial.print("Connecting to ");
    Serial.print(ssid);
    /* No pass for WiFi. We are looking for non-encrypteds. */
    WiFi.begin(ssid);
    unsigned short try_cnt = 0;
    /* Wait until WiFi connection but do not exceed MAX_CONNECT_TIME */
    while (WiFi.status() != WL_CONNECTED && try_cnt < MAX_CONNECT_TIME / WIFI_DELAY) {
      delay(WIFI_DELAY);
      Serial.print(".");
      try_cnt++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(F("\nConnection Successful!"));
      Serial.println(F("Your device IP address is "));
      Serial.println(WiFi.localIP());
    } else {
      Serial.println(F("\nOpen WiFi Connection Failed"));
    }
  } else {
    int wifiIndex = 0;
    while (WiFi.status() != WL_CONNECTED and wifiIndex < 2) {
      WiFi.begin(secureSSID[wifiIndex], securePass[wifiIndex]);
      delay(15000);
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print(F("\nConnected to SSID: "));
        Serial.println(secureSSID[wifiIndex]);
        return;
      }
      wifiIndex++;
    }
    Serial.println(F("\nSecure connection failed."));
  }
}

//Scan available networks and sort them in order to their signal strength
void scanAndSort() {
  memset(ssid, 0, MAX_SSID_LEN);
  int n = WiFi.scanNetworks();
  Serial.println("Scan complete!");
  if (n == 0) {
    Serial.println("No networks available.");
  } else {
    Serial.print(n);
    Serial.println(" networks discovered.");
    int indices[n];
    for (int i = 0; i < n; i++) {
      indices[i] = i;
    }
    for (int i = 0; i < n; i++) {
      for (int j = i + 1; j < n; j++) {
        if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
          std::swap(indices[i], indices[j]);
        }
      }
    }
    for (int i = 0; i < n; ++i) {
      if (WiFi.encryptionType(indices[i]) == ENC_TYPE_NONE) {
        Serial.println(F("The strongest open network is:"));
        Serial.print(F("SSID: "));
        Serial.println(WiFi.SSID(indices[i]));
        Serial.print(F("Strength: "));
        Serial.print(WiFi.RSSI(indices[i]));
        Serial.println("dB");
        Serial.print(F("EncryptionType: "));
        Serial.println(WiFi.encryptionType(indices[i]));

        memset(ssid, 0, MAX_SSID_LEN);
        strncpy(ssid, WiFi.SSID(indices[i]).c_str(), MAX_SSID_LEN);
        break;
      }
    }
  }
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
  if (icao == "KCVC") {
    pixel = 7;
  }
  else if (icao == "KWDR") {
    pixel = 11;
  }
  else if (icao == "KGVL") {
    pixel = 14;
  }
  else if (icao == "KLZU") {
    pixel = 17;
  }
  else if (icao == "KPDK") {
    pixel = 20;
  }
  else if (icao == "KRYY") {
    pixel = 22;
  }
  else if (icao == "KMGE") {
    pixel = 23;
  }
  else if (icao == "KFTY") {
    pixel = 24;
  }
  else if (icao == "KATL") {
    pixel = 26;
  }
  else if (icao == "KHMP") {
    pixel = 28;
  }
  else if (icao == "KFFC") {
    pixel = 30;
  }
  else if (icao == "KCCO") {
    pixel = 32;
  }
  else if (icao == "KLGC") {
    pixel = 35;
  }
  else if (icao == "KCTJ") {
    pixel = 40;
  }
  else if (icao == "KPUJ") {
    pixel = 43;
  }
  else if (icao == "KVPC") {
    pixel = 45;
  }
  else if (icao == "KCNI") {
    pixel = 48;
  }

  Serial.print(F("ICAO: "));
  Serial.println(icao);
  if (timeDifference > 7200) {
    pixels.setPixelColor(pixel, Color_Clear);
    Serial.println(F("****** STALE DATA OBSERVED"));
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

  Serial.print(F("Observed: "));
  Serial.println(observed);
  Serial.print(F("Difference: "));
  Serial.println(timeDifference);
  Serial.print(F("Condition: "));
  Serial.println(flight_category);
  Serial.println();
  pixels.show();
}

void initializeLights() {
  digitalWrite(LED_BUILTIN, LOW);
  pixels.begin();
  pixels.clear();
  pixels.setBrightness(50);
  pixels.setPixelColor(3, Color_VFR);
  pixels.setPixelColor(2, Color_MVFR);
  pixels.setPixelColor(1, Color_LIFR);
  pixels.setPixelColor(0, Color_IFR);
  pixels.show();
  Serial.println(F("Lights initialized"));
}
