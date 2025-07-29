#include <WiFi.h>
#include <HTTPClient.h>

// WiFi credentials
const char* ssid = "Croft_Wifi";
const char* password = "(N33r0n!12-11)";

// Supabase REST endpoint
const char* supabaseUrl = "https://nveofbxzhdksrjebijxu.supabase.co/rest/v1/rep_data";
const char* apiKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Im52ZW9mYnh6aGRrc3JqZWJpanh1Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NTE2NDQ5NzAsImV4cCI6MjA2NzIyMDk3MH0.xZ5BIzKXNM9i1FhIHtFT7SuMsEgZbZuWtNFfPk9VAus";

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
