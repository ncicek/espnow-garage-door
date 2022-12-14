#include <WiFi.h>
#include <esp_now.h>
#include "common.h"

#define BUTTON 0
#define LED 15

uint8_t serverAddress[] = SERVER_ADDRESS;

esp_now_send_status_t global_status;

#define TIMEOUT_MS 1000

void delay_sleep(uint32_t time_ms) {
  esp_sleep_enable_timer_wakeup(time_ms * 1000);
  esp_light_sleep_start();
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
}

void blink_led(uint16_t interval, uint8_t repetitions) {
  uint8_t i;
  for (i = 0; i < repetitions; i++) {
    digitalWrite(LED, 1);
    delay_sleep(interval);
    digitalWrite(LED, 0);
    if (repetitions > 1) {
      delay_sleep(interval);
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

  //wait for button release
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON, HIGH);
  esp_light_sleep_start();
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT0);
}

void hibernate() {
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH,   ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
  esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL,         ESP_PD_OPTION_OFF);
  esp_deep_sleep_start();
}

void setup() {
  pinMode(BUTTON, INPUT);
  pinMode(LED, OUTPUT);

  if (esp_reset_reason() == ESP_RST_DEEPSLEEP) {
    buttonPressed();
  }

  esp_sleep_enable_ext1_wakeup(1 << BUTTON, ESP_EXT1_WAKEUP_ALL_LOW);
  hibernate();
}

void loop() {
}