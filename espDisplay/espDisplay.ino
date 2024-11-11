#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <WiFiClientSecureBearSSL.h>

#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CS_PIN D8

const char* ssid = "RUIOT";
const char* password = "resnetiot";
const char* scriptURL = "https://script.google.com/macros/s/AKfycbxndxRpJ29Mns2UP6eGmnRTBPr-TF2woXHTAxteBmW8QZu4duAN3Repgwchj7uc4_2F/exec";  // Google Apps Script URL

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "time.nist.gov", 3600, 60000);
MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

struct Message {
  String timestamp;
  String text;
};

std::vector<Message> messages; // Store messages in a vector
int currentMessageIndex = 0;  // Track which message to display

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
  myDisplay.setIntensity(15);
  myDisplay.displayClear();
  myDisplay.setTextAlignment(PA_CENTER);
  myDisplay.print("Booting...");

  timeClient.begin();
  timeClient.setTimeOffset(-5);
}

void fetchMessages() {
  if (WiFi.status() == WL_CONNECTED) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
    client->setInsecure();

    HTTPClient http;
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

    Serial.print("[HTTPS] begin...\n");

    http.begin(*client, scriptURL);

    int httpCode = http.GET();

    if (httpCode != 0) {
      String payload = http.getString();
      Serial.println("JSON Response:");
      Serial.println(payload);

      http.end();

      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.print("Failed to parse JSON: ");
        Serial.println(error.c_str());
      } else {
        messages.clear();  // Clear previous messages
        for (JsonObject item : doc.as<JsonArray>()) {
          Message msg;
          msg.timestamp = item["timestamp"].as<String>();
          msg.text = item["message"].as<String>();
          messages.push_back(msg);
        }
        Serial.print("Loaded ");
        Serial.print(messages.size());
        Serial.println(" messages.");
      }
    } else {
      Serial.printf("Error: %d\n", httpCode);
    }
    http.end();
  } else {
    Serial.println("WiFi disconnected");
  }
}
void displayTime() {
  timeClient.update();
  unsigned long unix_epoch = timeClient.getEpochTime();

  // Set timezone offset (in seconds), e.g., -18000 for Eastern Time (GMT-5)
  int timezone_offset = -18000;
  unix_epoch += timezone_offset;

  int hour_ = hour(unix_epoch);
  int minute_ = minute(unix_epoch);
  int day_ = day(unix_epoch);
  int month_ = month(unix_epoch);
  int year_ = year(unix_epoch);

  // Adjust brightness based on the time of day
  if ((hour_ >= 8 && hour_ <= 18)) {   // Daytime hours
    myDisplay.setIntensity(15);        // Brighter during the day
  } else {
    myDisplay.setIntensity(2);         // Dimmer during the night
  }

  // Format time as HH:MM
  char timeString[6];
  snprintf(timeString, sizeof(timeString), "%02d:%02d", hour_, minute_);
  
  // Format date as MM/DD/YYYY
  char dateString[11];
  snprintf(dateString, sizeof(dateString), "%02d/%02d/%d", month_, day_, year_);

  Serial.print("Displaying time: ");
  Serial.println(timeString);
  Serial.print("Displaying date: ");
  Serial.println(dateString);

  // Display the time on the LED matrix
  myDisplay.displayClear();
  myDisplay.displayScroll(timeString, PA_CENTER, PA_SCROLL_LEFT, 50);
  while (!myDisplay.displayAnimate()) {
    yield();
  };

  // Display the date on the LED matrix
  myDisplay.displayClear();
  myDisplay.displayScroll(dateString, PA_CENTER, PA_SCROLL_LEFT, 50);
  while (!myDisplay.displayAnimate()) {
    yield();
  };
}



void displayCurrentMessage() {
  if (messages.empty()) return; // Exit if there are no messages

  Message &msg = messages[currentMessageIndex];
  Serial.print("Displaying message: ");
  Serial.println(msg.text);

  myDisplay.displayClear();
  myDisplay.displayScroll(msg.text.c_str(), PA_CENTER, PA_SCROLL_LEFT, 50);
  
  while (!myDisplay.displayAnimate()) {
    yield(); // Prevents WDT reset by giving control back to the system
  }

  Serial.println("Done");

  // if the currentMessageIndex = messages.size() - 1
  // print time
  if (currentMessageIndex == messages.size() - 1) {
    displayTime();
  }
  
  // Move to the next message
  currentMessageIndex = (currentMessageIndex + 1) % messages.size();
}

void updateBrightness() {

}

void loop() {
  static unsigned long lastFetchTime = 0;
  static unsigned long lastDisplayTime = 0;

  // Fetch new messages every 5 minutes
  if (millis() - lastFetchTime > 30000) {
    fetchMessages();
    lastFetchTime = millis();
  }

  // Display a new message every 10 seconds
  if (millis() - lastDisplayTime > 100) {
    displayCurrentMessage();
    lastDisplayTime = millis();
    updateBrightness();
  }
}
