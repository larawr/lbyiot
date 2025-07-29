#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// WiFi credentials
const char* ssid = "Croft_Wifi";
const char* password = "(N33r0n!12-11)";

// Supabase
const char* supabaseUrl = "https://nveofbxzhdksrjebijxu.supabase.co";
const char* apiKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Im52ZW9mYnh6aGRrc3JqZWJpanh1Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NTE2NDQ5NzAsImV4cCI6MjA2NzIyMDk3MH0.xZ5BIzKXNM9i1FhIHtFT7SuMsEgZbZuWtNFfPk9VAus"; // shortened for safety
const char* repEndpoint = "/rest/v1/rep_data";
const char* configEndpoint = "/rest/v1/device_config?device_id=eq.esp32-001&select=user,equipment";

// Sensor
const int sensorPin = 4;
int count = 0;
bool lastState = LOW;

// Config state
String username = "";
String equipmentType = "";
String lastUsername = "";
String lastEquipment = "";

unsigned long lastPoll = 0;
const unsigned long pollInterval = 5000;

void setup() {
  Serial.begin(115200);
  pinMode(sensorPin, INPUT);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n✅ Connected to WiFi");
}

void loop() {
  // Fetch config every 5 seconds
  if (millis() - lastPoll > pollInterval) {
    fetchDeviceConfig();
    lastPoll = millis();
  }

  // Only count if config loaded
  if (username != "" && equipmentType != "") {
    bool motion = digitalRead(sensorPin);
    if (motion == HIGH && lastState == LOW) {
      count++;
      Serial.printf("Rep Count: %d\n", count);
      sendToSupabase(count, equipmentType, username);
      delay(1000); // debounce
    }
    lastState = motion;
  }

  delay(50);
}

void fetchDeviceConfig() {
  HTTPClient http;
  String fullUrl = String(supabaseUrl) + configEndpoint;

  http.begin(fullUrl);
  http.addHeader("apikey", apiKey);
  http.addHeader("Authorization", "Bearer " + String(apiKey));

  int code = http.GET();
  if (code == 200) {
    String payload = http.getString();
    DynamicJsonDocument doc(512);
    deserializeJson(doc, payload);
    if (doc.size() > 0) {
      String newUser = doc[0]["user"].as<String>();
      String newEquipment = doc[0]["equipment"].as<String>();

      if (newUser != username || newEquipment != equipmentType) {
        Serial.printf("🔄 New config detected: %s, %s — resetting count!\n", newUser.c_str(), newEquipment.c_str());
        count = 0;
      }

      username = newUser;
      equipmentType = newEquipment;

      Serial.println("📥 Config: " + username + ", " + equipmentType);
    }
  } else {
    Serial.printf("❌ Failed to fetch config: %d\n", code);
  }

  http.end();
}

void sendToSupabase(int repCount, String equipmentType, String username) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(String(supabaseUrl) + repEndpoint);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", apiKey);
    http.addHeader("Authorization", "Bearer " + String(apiKey));
    http.addHeader("Prefer", "return=minimal");

    String jsonPayload = "{\"count\":" + String(repCount) +
                         ",\"type\":\"" + equipmentType +
                         "\",\"user\":\"" + username + "\"}";

    Serial.println("Sending payload: " + jsonPayload);
    int httpResponseCode = http.POST(jsonPayload);

    Serial.print("HTTP Response Code: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode > 0) {
      Serial.println("✅ Data sent!");
    } else {
      Serial.printf("❌ Error: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  }
}
