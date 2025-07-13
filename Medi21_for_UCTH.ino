#include <Ticker.h>
#include <PxMatrix.h> //https://github.com/2dom/PxMatrix
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <WiFiManager.h> // Include WiFiManager library
#include <ArduinoOTA.h> 
#include <ESP8266mDNS.h>
#define SECRET_CH_ID_WEATHER_STATION 12397              //MathWorks weather station
#define SECRET_CH_ID_COUNTER 2737001          //Test channel for counting
#define SECRET_READ_APIKEY_COUNTER "P1823CW9MUPZN81Z" //API Key for Test channel

Ticker display_ticker;

// API details
const char* host = "api.mediboards.io";
const String path = "/api/public/hospitals/be6e6f64-9d3b-4c1e-a6a3-910e4495792c/latest-patient";
String count;

WiFiClient client;

// Pin Definition for Nodemcu to HUB75 LED MODULE
#define P_LAT 16 //nodemcu pin D0
#define P_A 5    //nodemcu pin D1
#define P_B 4    //nodemcu pin D2
#define P_C 15   //nodemcu pin D8
#define P_OE 2   //nodemcu pin D4
#define P_D 12   //nodemcu pin D6
#define P_E 0    //nodemcu pin GND // no connection

PxMATRIX display(256, 32, P_LAT, P_OE, P_A, P_B, P_C, P_D);

// Single color (Red)
uint16_t myRED = display.color565(255, 0, 0);

// ISR for display refresh
void display_updater() {
  //display.display(100);
   display.display(100);
}

// OTA event handlers
void setupOTA() {
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
  Serial.println("OTA Ready");
}

void fetchAndDisplayData() {
  WiFiClientSecure client;
  HTTPClient https;

  // Skip certificate validation (use with caution)
  client.setInsecure();

  // Configure timeout
  https.setTimeout(10000);

  // Begin HTTPS connection
  if (https.begin(client, String("https://") + host + path)) {
    Serial.println("Fetching data from API...");

    // Send GET request
    int httpCode = https.GET();

    if (httpCode > 0) {
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        // Read the response
        count = https.getString();
        Serial.println("API Response:");
        Serial.println(count);

        // Parse JSON response
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, count);

        if (error) {
          Serial.print("JSON parsing failed: ");
          Serial.println(error.c_str());
          return;
        }

        // Extract data from JSON
        const char* patientName = doc["patientName"];
        const char* bedNumber = doc["bedNumber"];
        const char* status = doc["status"];

        // Display data on P4 LED matrix
    
        Serial.println("Data displayed on LED matrix");
      } else {
        Serial.printf("HTTP GET failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
    } else {
      Serial.printf("HTTP GET failed, error: %s\n", https.errorToString(httpCode).c_str());
    }

    https.end();
  } else {
    Serial.println("Unable to connect to API");
  }
}

void setup() {
  // Initialize display
  display.begin(16);
  display.clearDisplay();
  display.setTextColor(myRED); // Set default text color to Red
  Serial.begin(115200);

  // Initialize WiFiManager
  WiFiManager wifiManager;
display.setFastUpdate(true); // Add this in setup()
  // Uncomment the following line to reset saved settings (for testing)
  // wifiManager.resetSettings();

  // Set a timeout for the configuration portal
  wifiManager.setTimeout(180); // 3 minutes

  // Attempt to connect to Wi-Fi or launch the configuration portal
  if (!wifiManager.autoConnect("UCTH MediboardsAP")) {
    Serial.println("Failed to connect and hit timeout");
    delay(3000);
    ESP.reset(); // Reset and try again
    delay(5000);
  }
setupOTA();
  Serial.println("Connected to WiFi");
 fetchAndDisplayData();
  // Start display ticker
  display_ticker.attach(0.008, display_updater);

  // Display initial message
  display.clearDisplay();
  display.setCursor(55, 1);
  display.setTextSize(2);
  display.setTextColor(myRED); // Use Red color
  display.print(" MEDIBOARDS ");
  display.setCursor(5, 17);
  display.setTextSize(2);
  display.setTextColor(myRED); // Use Red color
  display.print("POWERED BY SONVISAGE ");
  delay(6000);

  // Check API
   
}

void scroll_text(uint8_t ypos, unsigned long scroll_delay, String text) {
  uint16_t text_length = text.length();
  display.setTextWrap(false);
  //display.setTextSize(2); // Increase text size to 2 (double the default size)
  //display.setRotation(0);
  //display.setTextColor(myRED); // Use Red color
  display.clearDisplay();
  // Draw static text once (outside the loop)
  display.setCursor(15, 1);
  
  display.setTextColor(myRED); // Use Red color
  display.print("UCTH EMERGENCY DEPT.");
   display.setTextSize(2); // Size for static text
  for (int xpos = 192; xpos > -(32 + text_length * 10); xpos--) { // Adjusted for larger text size
    // Clear only the area where the scrolling text is drawn
    display.fillRect(xpos, ypos, (text_length+10) * 10, 16, 0); // Clear previous text with black color

    // Draw scrolling text
    display.setCursor(xpos, ypos);
    display.println(text);

    yield();

  }

 
  
}

void loop() {
ArduinoOTA.handle();
fetchAndDisplayData();
Serial.println("count");
 Serial.println(count);
 //count="B12";
  if(count !=""){
    display.clearDisplay();
    display.setCursor(60, 1);
    display.setTextSize(2);
    display.setTextColor(myRED);
    display.print("NEW PATIENT ");
    display.setCursor(60, 17);
    display.print("AT COUCH " + (count));
    
     delay(15000);
  display.setCursor(1, 1);
     String txt = ("NEW PATIENT AT COUCH "+ (count));
  //This will display the scrolling text.
     scroll_text(17, 0, String(txt));
     scroll_text(17, 0, String(txt));
     scroll_text(17, 0, String(txt));
    //scroll_text(y-pos, delay, "TEXT", R, G, B); 
    display.clearDisplay();
    }
    else {
      display.clearDisplay();
    display.setCursor(40, 1);
    display.setTextSize(2);
    display.setTextColor(myRED);
    display.print("WELCOME TO UCTH");
    display.setCursor(0, 17);
    display.print("EMERGENCY DEPT. ");
    //CheckApi();
     delay(15000);
 
  display.setCursor(1, 1);
     String txt = ("OUR MOTTO: SERVICE, INTERGRITY, EMPATHY AND INNOVATION ");
  //This will display the scrolling text.
     scroll_text(17, 0, String(txt));
     scroll_text(17, 0, String(txt));
     scroll_text(17, 0, String(txt));
    //scroll_text(y-pos, delay, "TEXT", R, G, B); 
    display.clearDisplay();
      
      }
      }
