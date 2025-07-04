#include <WiFi.h>
#include <HTTPClient.h>

// WiFi credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Supabase REST endpoint
const char* supabaseUrl = "https://YOUR_PROJECT.supabase.co/rest/v1/rep_data";
const char* apiKey = "YOUR_SUPABASE_ANON_KEY";

// Motion sensor pin
const int sensorPin = 5; // GPIO5
int count = 0;
bool lastState = LOW;

void setup() {
  Serial.begin(115200);
  pinMode(sensorPin, INPUT);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
}

void loop() {
  bool motion = digitalRead(sensorPin);

  // Simple edge detection to avoid multiple counts per motion
  if (motion == HIGH && lastState == LOW) {
    count++;
    Serial.printf("Rep Count: %d\n", count);
    sendToSupabase(count);
    delay(1000); // debounce delay
  }

  lastState = motion;
  delay(50); // sampling rate
}

void sendToSupabase(int repCount) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    http.begin(supabaseUrl);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", apiKey);
    http.addHeader("Authorization", "Bearer " + String(apiKey));
    http.addHeader("Prefer", "return=minimal");

    String jsonPayload = "{\"count\":" + String(repCount) + "}";

    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      Serial.printf("Data sent: %d\n", httpResponseCode);
    } else {
      Serial.printf("Error sending data: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}
