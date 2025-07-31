#include <esp_now.h>
#include <WiFi.h>

// Rep counter
const int sensorPin = 21;  // GPIO pin for motion sensor
int count = 0;
bool lastState = LOW;
unsigned long lastMotionTime = 0;  // Variable to debounce motion sensor

// ESP-NOW message structure (including device_id, count)
typedef struct struct_message {
  int count;
  char device_id[18];  // MAC address length (18 characters max for MAC address as a string)
} struct_message;

struct_message outgoingData;

// MAC address of the second ESP32 (WiFi ESP32) – Replace with the actual MAC address of your Wi-Fi ESP32
uint8_t receiverMAC[] = {0x5C, 0x01, 0x3B, 0x64, 0xE0, 0xC0};  // Example MAC address (replace with the correct one)

void setup() {
  // Start serial communication for debugging
  Serial.begin(115200);
  pinMode(sensorPin, INPUT);  // Set the sensor pin as input

  // Log the start of ESP-NOW initialization
  Serial.println("ESP-NOW Sender Init");

  // Initialize Wi-Fi in station mode
  WiFi.mode(WIFI_STA);  // Set ESP32 to Station mode
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ Wi-Fi not connected! Please check the connection.");
    return;
  }
  Serial.println("✅ Wi-Fi Connected!");

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ ESP-NOW init failed");
    return;  // If ESP-NOW init fails, stop further execution
  }

  // Set up peer (Device 2) for ESP-NOW communication
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);  // Device 2's MAC address
  peerInfo.channel = 0;  // Default channel
  peerInfo.encrypt = false;  // No encryption

  // Add peer to the ESP-NOW network
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("❌ Failed to add peer");
    return;  // If adding peer fails, stop further execution
  }

  // Get and store the MAC Address of Device 1 (Sensor ESP32)
  String deviceMAC = WiFi.macAddress();
  deviceMAC.toCharArray(outgoingData.device_id, 18);  // Store MAC address in device_id
  Serial.println("Device MAC Address: " + deviceMAC);  // Print Device MAC Address to Serial Monitor

  // Log ESP-NOW ready status
  Serial.println("✅ ESP-NOW ready");
}

void loop() {
  bool motion = digitalRead(sensorPin);  // Read the motion sensor

  // Print the raw state of the motion sensor to the Serial Monitor
  Serial.println("Sensor state: " + String(motion));  // This will print 1 (HIGH) or 0 (LOW) based on sensor state

  // Only print "Motion detected!" once when motion starts
  if (motion == HIGH && lastState == LOW) {
    Serial.println("Motion detected!");  // Print when motion is detected
    count++;  // Increase count if motion detected
    Serial.printf("Rep Count: %d\n", count);  // Log the rep count to Serial Monitor

    // Prepare data to send over ESP-NOW
    outgoingData.count = count;
    Serial.printf("Preparing to send: count = %d, device_id = %s\n", count, outgoingData.device_id);

    // Send the data to Device 2 via ESP-NOW
    esp_now_send(receiverMAC, (uint8_t *)&outgoingData, sizeof(outgoingData));

    // Log the sending action
    Serial.println("Data sent to Device 2 via ESP-NOW.");

    // Set the last motion time to debounce motion sensor (ignore repeated signals for 1 second)
    lastMotionTime = millis();
    Serial.println("Debouncing motion for 1 second...");
    delay(1000);  // Delay to debounce the motion sensor
  }

  // Log "No motion detected" after motion stops
  if (motion == LOW && lastState == HIGH) {
    Serial.println("No motion detected.");
  }

  lastState = motion;  // Update lastState for edge detection

  // Check for repeated motion detection too quickly (debounce)
  if (millis() - lastMotionTime < 1000) {
    Serial.println("Waiting for debounce...");
  }

  delay(50);  // Small delay to adjust the sampling rate of the motion sensor
}
