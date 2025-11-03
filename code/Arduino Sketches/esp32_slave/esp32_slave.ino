#include <WiFi.h>
#include <esp_now.h>

#define ESPNOW_CHANNEL 6

// Callback for receiving ESP-NOW messages
void onReceive(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    const uint8_t *mac = recv_info->src_addr;
    Serial.printf("Received from %02X:%02X:%02X:%02X:%02X:%02X: %.*s\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], len, data);
}

void setup() {
    Serial.begin(115200);

    WiFi.mode(WIFI_STA);
    WiFi.setChannel(ESPNOW_CHANNEL);

    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
        while (1);
    }

    // Register the callback 
    esp_now_register_recv_cb(onReceive);

    Serial.println("Slave ready. Waiting for master messages...");
}

void loop() {
    delay(100);
}
