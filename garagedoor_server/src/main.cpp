#include <WiFi.h>
#include <esp_now.h>
#include "common.h"

#define PIN 33
#define LED 15

// Set the SLAVE MAC Address
uint8_t clientAddress[] = CLIENT_ADDRESS;

void openDoor() {
  digitalWrite(PIN, 1);
  digitalWrite(LED, 1);
  delay(450);
  digitalWrite(PIN, 0);
  digitalWrite(LED, 0);
}

uint32_t data;

// callback function executed when data is received
void OnRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&data, incomingData, sizeof(data));
  if (data == 0xDEADBEEF) {
    openDoor();
  }
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);

  pinMode(PIN, OUTPUT);
  pinMode(LED, OUTPUT);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("There was an error initializing ESP-NOW");
    return;
  }

  // Setting the PMK key
  esp_now_set_pmk((uint8_t *)PMK_KEY_STR);

  // Register the master
  esp_now_peer_info_t clientInfo;
  memset(&clientInfo, 0, sizeof(clientInfo));
  memcpy(clientInfo.peer_addr, clientAddress, 6);
  clientInfo.channel = 0;
  // Setting the master device LMK key
  for (uint8_t i = 0; i < 16; i++) {
    clientInfo.lmk[i] = LMK_KEY_STR[i];
  }
  clientInfo.encrypt = true;
  
  // Add master        
  if (esp_now_add_peer(&clientInfo) != ESP_OK){
    Serial.println("There was an error registering the master");
    return;
  }
  
  // Once the ESP-Now protocol is initialized, we will register the callback function
  // to be able to react when a package arrives in near to real time without pooling every loop.
  esp_now_register_recv_cb(OnRecv);
}

void loop() {
}