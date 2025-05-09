#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// === WiFi Credentials ===
const char* ssid = "ssid"; 
const char* password = "password";

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected.");
}

void loop() {
  float price = 0.0;
  float change = 0.0;

  if (fetchBTCData(price, change)) {
    Serial.printf("BTC Price: $%.2f | 24h Change: %.2f%%\n", price, change);
  } else {
    Serial.println("Failed to get BTC data.");
  }

  delay(60000); // Check every 10 seconds
}

bool fetchBTCData(float &priceOut, float &percentOut) {
  HTTPClient http;
  http.begin("https://api.coingecko.com/api/v3/coins/bitcoin?localization=false&tickers=false&market_data=true");

  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      priceOut = doc["market_data"]["current_price"]["usd"];
      percentOut = doc["market_data"]["price_change_percentage_24h"];
      return true;
    } else {
      Serial.println("JSON parsing failed.");
    }
  } else {
    Serial.printf("HTTP error: %d\n", httpCode);
  }

  return false;
}
