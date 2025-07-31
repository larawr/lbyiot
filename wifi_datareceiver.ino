#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_now.h>
#include <ArduinoJson.h>

// Wi-Fi Credentials
const char* ssid = "Croft_Wifi";
const char* password = "(N33r0n!12-11)";

// Supabase Config
const char* supabaseUrl = "https://nveofbxzhdksrjebijxu.supabase.co";
const char* apiKey = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6Im52ZW9mYnh6aGRrc3JqZWJpanh1Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NTE2NDQ5NzAsImV4cCI6MjA2NzIyMDk3MH0.xZ5BIzKXNM9i1FhIHtFT7SuMsEgZbZuWtNFfPk9VAus";
const char* repEndpoint = "/rest/v1/rep_data";
const char* configEndpoint = "/rest/v1/device_config?device_id=eq.";  // We’ll append device_id here

// Rep counter
int lastRep = 0;
int count = 0;

// Config from Supabase (will be fetched dynamically)
String username = "";
String equipmentType = "";

// ESP-NOW message structure
typedef struct struct_message {
  int count;
  char device_id[18];  // MAC address length
} struct_message;

struct_message incomingData;  // Struct to hold incoming data from the sensor ESP32

// Callback function to handle incoming ESP-NOW data
void OnDataRecv(const esp_now_recv_info* info, const uint8_t* incomingData, int len) {
  struct_message incoming;
  memcpy(&incoming, incomingData, sizeof(incoming));  // Copy the incoming data to the struct
  int rep = incoming.count;
  String deviceIdReceived = incoming.device_id;

  // Log received data
  Serial.printf("Received data from device: %s\n", deviceIdReceived);
  Serial.printf("Received rep count: %d\n", rep);

  // Check if the rep count has changed
  if (rep != lastRep) {
    lastRep = rep;
    count = rep;
    Serial.println("Rep count updated");
    fetchDeviceConfig(deviceIdReceived);  // Fetch config for this device from Supabase
  } else {
    Serial.println("Rep count unchanged, not fetching config.");
  }
}

void fetchDeviceConfig(String deviceId) {
  Serial.println("Fetching device configuration from Supabase...");

  HTTPClient http;
  String fullUrl = String(supabaseUrl) + String(configEndpoint) + deviceId;

  http.begin(fullUrl);
  http.addHeader("apikey", apiKey);
  http.addHeader("Authorization", "Bearer " + String(apiKey));

  // Log the URL being fetched
  Serial.println("Fetching URL: " + fullUrl);

  int code = http.GET();
  Serial.printf("HTTP Status Code: %d\n", code);  // Log the HTTP status code

  if (code == 200) {
    String payload = http.getString();
    Serial.println("Config fetched successfully!");

    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.print("JSON Deserialization failed: ");
      Serial.println(error.f_str());
      return;  // Exit if JSON parsing failed
    }

    if (doc.size() > 0) {
      username = doc[0]["user"].as<String>();
      equipmentType = doc[0]["equipment"].as<String>();
      Serial.println("Config: " + username + ", " + equipmentType);
      sendToSupabase(count, equipmentType, username, deviceId);  // Send rep count to Supabase
    } else {
      Serial.println("No configuration found for device: " + deviceId);
    }
  } else {
    Serial.printf("Failed to fetch config: %d\n", code);
  }

  http.end();
}

void sendToSupabase(int repCount, String equipmentType, String username, String deviceId) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(supabaseUrl) + repEndpoint;

    // Log sending process
    Serial.println("📤 Sending rep data to Supabase...");

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("apikey", apiKey);
    http.addHeader("Authorization", "Bearer " + String(apiKey));

    String jsonPayload = "{\"count\":" + String(repCount) +
                         ",\"type\":\"" + equipmentType +
                         "\",\"user\":\"" + username +
                         "\",\"device_id\":\"" + deviceId + "\"}";

    // Log the JSON payload being sent
    Serial.println("📤 Sending to Supabase: " + jsonPayload);
    int httpResponseCode = http.POST(jsonPayload);

    // Log the HTTP response code
    Serial.print("HTTP Response Code: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode > 0) {
      Serial.println("Data sent to Supabase!");
    } else {
      Serial.printf("Error: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
  } else {
    Serial.println("Wi-Fi not connected! Cannot send data.");
  }
}

void setup() {
  Serial.begin(115200);  // Start serial communication
  WiFi.begin(ssid, password);  // Connect to Wi-Fi

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("✅ Connected to Wi-Fi");

  // Log Wi-Fi connection
  Serial.println("Wi-Fi Connected!");

  // Initialize ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW initialization failed");
    return;
  }

  // Set up peer device (sensor ESP32)
  esp_now_peer_info_t peerInfo = {};
  // In the receiver (Wi-Fi ESP32), you do **not** need to define the sensor's MAC address for self-communication!
  // The peerInfo only needs to be added in the sender (sensor ESP32) code.
  peerInfo.channel = 0;  // Default channel
  peerInfo.encrypt = false;  // No encryption

  // Register the ESP-NOW callback function to handle received data
  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("ESP-NOW Initialized and Peer Added");
}

void loop() {
  delay(50);  // Wait for incoming ESP-NOW messages
}
