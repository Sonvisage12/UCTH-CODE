#include <Ticker.h>
#include <PxMatrix.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>

#define SECRET_CH_ID_WEATHER_STATION 12397
#define SECRET_CH_ID_COUNTER 2737001
#define SECRET_READ_APIKEY_COUNTER "P1823CW9MUPZN81Z"

Ticker display_ticker;

const char* host = "api.mediboards.io";
const String path = "/api/public/hospitals/be6e6f64-9d3b-4c1e-a6a3-910e4495792c/latest-patient";
String count;

WiFiClient client;

// Pin Definition for Nodemcu to HUB75 LED MODULE
#define P_LAT 16
#define P_A 5
#define P_B 4
#define P_C 15
#define P_OE 2
#define P_D 12
#define P_E 0

PxMATRIX display(256, 32, P_LAT, P_OE, P_A, P_B, P_C, P_D);
uint16_t myRED = display.color565(255, 0, 0);

void display_updater() {
  display.display(100);
}

void setupOTA() {
  ArduinoOTA.setHostname("mediboards-display");

  ArduinoOTA.onStart([]() {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(myRED);
    display.println("OTA Start");
    display.display();
  });

  ArduinoOTA.onEnd([]() {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("OTA End");
    display.display();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("OTA: %u%%", (progress / (total / 100)));
    display.display();
  });

  ArduinoOTA.onError([](ota_error_t error) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.printf("OTA Error [%u]", error);
    display.display();
  });

  ArduinoOTA.begin();
  MDNS.begin("mediboards-display");  // Enable mDNS for OTA visibility
  Serial.println("✅ OTA Ready");
}

void fetchAndDisplayData() {
  WiFiClientSecure client;
  HTTPClient https;
  client.setInsecure();
  https.setTimeout(10000);

  if (https.begin(client, String("https://") + host + path)) {
    Serial.println("Fetching data from API...");
    int httpCode = https.GET();

    if (httpCode > 0 && httpCode == HTTP_CODE_OK) {
      count = https.getString();
      Serial.println("API Response:");
      Serial.println(count);

      DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, count);

      if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return;
      }

    } else {
      Serial.printf("HTTP GET failed: %s\n", https.errorToString(httpCode).c_str());
    }

    https.end();
  } else {
    Serial.println("Unable to connect to API");
  }
}

void setup() {
  Serial.begin(115200);
  display.begin(16);
  display.setFastUpdate(true);
  display.clearDisplay();
  display.setTextColor(myRED);

  WiFiManager wifiManager;
  wifiManager.setTimeout(180);
  if (!wifiManager.autoConnect("UCTH MediboardsAP")) {
    Serial.println("Failed to connect. Rebooting...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("Connected to WiFi");
  setupOTA();
  fetchAndDisplayData();
  display_ticker.attach(0.008, display_updater);

  display.clearDisplay();
  display.setCursor(55, 1);
  display.setTextSize(2);
  display.setTextColor(myRED);
  display.print(" MEDIBOARDS ");
  display.setCursor(5, 17);
  display.print("POWERED BY SONVISAGE ");
  delay(4000);
}

void scroll_text(uint8_t ypos, unsigned long scroll_delay, String text) {
  uint16_t text_length = text.length();
  display.setTextWrap(false);
  display.clearDisplay();
  display.setCursor(15, 1);
  display.setTextColor(myRED);
  display.print("UCTH EMERGENCY DEPT.");
  display.setTextSize(2);

  for (int xpos = 192; xpos > -(32 + text_length * 10); xpos--) {
    ArduinoOTA.handle();  // Make OTA responsive even during scrolling
    display.fillRect(0, ypos, 256, 16, 0); // Clear scrolling area
    display.setCursor(xpos, ypos);
    display.println(text);
    delay(20); // Smooth scrolling delay
  }
}

void loop() {
  ArduinoOTA.handle();

  fetchAndDisplayData();
  Serial.println("count: " + count);

  if (count != "") {
    display.clearDisplay();
    display.setCursor(60, 1);
    display.setTextSize(2);
    display.setTextColor(myRED);
    display.print("NEW PATIENT ");
    display.setCursor(60, 17);
    display.print("AT COUCH " + count);
    delay(10000);

    String txt = "NEW PATIENT AT COUCH " + count;
    scroll_text(17, 0, txt);
    display.clearDisplay();

  } else {
    display.clearDisplay();
    display.setCursor(40, 1);
    display.setTextSize(2);
    display.setTextColor(myRED);
    display.print("WELCOME TO UCTH");
    display.setCursor(0, 17);
    display.print("EMERGENCY DEPT.");
    delay(10000);

    String txt = "OUR MOTTO: SERVICE, INTEGRITY, EMPATHY AND INNOVATION";
    scroll_text(17, 0, txt);
    display.clearDisplay();
  }
}
