#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);  // Set Wi-Fi to Station mode
  Serial.println(WiFi.macAddress());  // Prints the MAC address to the serial monitor
}

void loop() {
  // Nothing to do in the loop
}
