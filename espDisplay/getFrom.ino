#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Arduino.h>
#include <WiFiClientSecureBearSSL.h>

// WiFi credentials
const char* ssid = "";
const char* password = "";

// URL to fetch JSON data from
const char* json_url = "https://script.google.com/macros/s/AKfycbxndxRpJ29Mns2UP6eGmnRTBPr-TF2woXHTAxteBmW8QZu4duAN3Repgwchj7uc4_2F/exec";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");

  // Connect to WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);

    // Ignore SSL certificate validation
    client->setInsecure();
    
    //create an HTTPClient instance
    HTTPClient http;
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

    //Initializing an HTTPS communication using the secure client
    Serial.print("[HTTPS] begin...\n");

    http.begin(*client, json_url); // Initialize HTTP client with the URL

    int httpCode = http.GET(); // Make the GET request

    if (httpCode != 0) {
      String payload = http.getString(); // Get the JSON response as a string

      // Print entire JSON response
      Serial.println("JSON Response:");
      Serial.println(payload);

      // Parse JSON
      StaticJsonDocument<1024> doc; // Adjust size based on JSON size
      DeserializationError error = deserializeJson(doc, payload);

      if (error) {
        Serial.print("Failed to parse JSON: ");
        Serial.println(error.c_str());
      } else {
        Serial.print("got it");
        // Iterate over each element in the JSON array
        for (JsonObject item : doc.as<JsonArray>()) {
          const char* timestamp = item["timestamp"];
          const char* message = item["message"];

          Serial.print("Timestamp: ");
          Serial.println(timestamp);
          Serial.print("Message: ");
          Serial.println(message);
          Serial.println();
        }
      }
    } else {
      Serial.print("HTTP request failed, error: ");
      Serial.println(httpCode);
      Serial.println(http.getString());
    }

    http.end(); // Close connection
  } else {
    Serial.println("WiFi not connected");
  }

  delay(10000); // Delay between requests
}
