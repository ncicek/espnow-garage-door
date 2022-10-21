#include <WiFi.h>
#include <esp_now.h>
#include "common.h"

#define BUTTON 0
#define LED 15

uint8_t serverAddress[] = SERVER_ADDRESS;

RTC_DATA_ATTR int bootCount = 0;
esp_now_send_status_t global_status;

#define TIMEOUT_MS 1000

void blink_led(uint16_t interval, uint8_t repetitions) {
  uint8_t i;
  for (i = 0; i < repetitions; i++) {
    digitalWrite(LED, 1);
    delay(interval);
    digitalWrite(LED, 0);
    if (repetitions > 1) {
      delay(interval);
    }
  }
}

// Callback to have a track of sent messages
void OnSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  global_status = status;
}

void buttonPressed() {
  const uint32_t data = 0xDEADBEEF;

  uint16_t timeout = 0;

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    return;
  }

  // Setting the PMK key
  esp_now_set_pmk((uint8_t *)PMK_KEY_STR);
  
  // Register the slave
  esp_now_peer_info_t serverInfo;
  memset(&serverInfo, 0, sizeof(serverInfo));
  memcpy(serverInfo.peer_addr, serverAddress, 6);
  serverInfo.channel = 0;
  // Setting the master device LMK key
  for (uint8_t i = 0; i < 16; i++) {
    serverInfo.lmk[i] = LMK_KEY_STR[i];
  }
  serverInfo.encrypt = true;
  
  // Add slave        
  if (esp_now_add_peer(&serverInfo) != ESP_OK){
    esp_now_deinit();
    return;
  }

  global_status = ESP_NOW_SEND_FAIL;
  esp_now_register_send_cb(OnSent);
  esp_now_send(serverAddress, (uint8_t *) &data, sizeof(data));

  while(global_status != ESP_NOW_SEND_SUCCESS && timeout <= TIMEOUT_MS) {
    delay(1);
    timeout++;
  }
  
  esp_now_deinit();
  WiFi.mode(WIFI_OFF);

  if (global_status == ESP_NOW_SEND_SUCCESS) {
    blink_led(100,2);
  } else {
    blink_led(30,10);
  }
}

void setup() {
  pinMode(BUTTON, INPUT_PULLUP);
  pinMode(LED, OUTPUT);

  gpio_hold_en((gpio_num_t)BUTTON); // Hold the high state during deepsleep.

  if (bootCount != 0) {
    buttonPressed();
  }
  bootCount++;
  
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON, LOW);
  esp_deep_sleep_start();
}

void loop() {
}