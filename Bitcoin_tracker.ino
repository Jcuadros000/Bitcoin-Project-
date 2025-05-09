#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "ssid";
const char* password = "password";

void fetchAndSendBTCData() {
  HTTPClient http;
  http.begin("https://api.coingecko.com/api/v3/coins/bitcoin?localization=false&tickers=false&market_data=true");
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      float price = doc["market_data"]["current_price"]["usd"];
      float change = doc["market_data"]["price_change_percentage_24h"];

      char message[128];
      snprintf(message, sizeof(message), "BTC Price: $%.2f, 24h Change: %.2f%%\n", price, change);
      Serial.print(message);
    } else {
      Serial.println("JSON parsing error.");
    }
  } else {
    Serial.printf("HTTP error: %d\n", httpCode);
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Immediately fetch and send BTC data on startup
  fetchAndSendBTCData();
}

void loop() {
  // Fetch and send BTC data every 60 seconds thereafter
  delay(20000);
  fetchAndSendBTCData();
}
