#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>

// Configuration for LED Matrix
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN D8

const char *ssid = "RUIOT";
const char *password = "resnetiot";
const char* scriptURL = "https://script.google.com/macros/s/AKfycbxndxRpJ29Mns2UP6eGmnRTBPr-TF2woXHTAxteBmW8QZu4duAN3Repgwchj7uc4_2F/exec";  // Google Apps Script URL

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "time.nist.gov", 3600, 60000);
MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  // Connect to WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize display
  myDisplay.begin();
  myDisplay.setIntensity(0);
  myDisplay.displayClear();
  myDisplay.setTextAlignment(PA_CENTER);
  myDisplay.print("Booting...");

  timeClient.begin();
}

void displayTime() {
  timeClient.update();
  unsigned long unix_epoch = timeClient.getEpochTime();

  int hour_ = hour(unix_epoch);
  int minute_ = minute(unix_epoch);

  // Format time as HH:MM
  char timeString[6];
  snprintf(timeString, sizeof(timeString), "%02d:%02d", hour_, minute_);
  Serial.print("Displaying time: ");
  Serial.println(timeString);

  // Display the time on the LED matrix
  myDisplay.displayClear();
  myDisplay.displayScroll(timeString, PA_CENTER, PA_SCROLL_LEFT, 50);
  while (!myDisplay.displayAnimate());
}

void loop() {
  bool hasMessages = false;  // Track if any messages were received

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;

    http.begin(client, scriptURL);
    int httpResponseCode = http.GET();
    Serial.println(http.getString());

    if (httpResponseCode == 200) {
      String payload = http.getString();
      Serial.println("Data received:");
      Serial.println(payload);

      // Parse JSON
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, payload);

      myDisplay.displayClear();

      // Check if there are messages in the JSON array
      if (doc.size() > 0) {
        hasMessages = true;
        for (JsonObject msg : doc.as<JsonArray>()) {
          const char* message = msg["message"];
          Serial.printf("Message: %s\n", message);

          // Display each message on the matrix
          myDisplay.displayClear();
          myDisplay.displayScroll(message, PA_CENTER, PA_SCROLL_LEFT, 50);
          while (!myDisplay.displayAnimate());  // Wait until the message is fully displayed
        }
      }
    } else {
      Serial.printf("Error: %d\n", httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi disconnected");
  }

  // If no messages were displayed, show the current time
  if (!hasMessages) {
    displayTime();
  }

  delay(60000);  // Fetch new data every 60 seconds
}
