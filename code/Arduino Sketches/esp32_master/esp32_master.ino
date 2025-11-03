#include <Wire.h>
#include "ESP32_NOW.h"
#include "WiFi.h"

#define ACCEL_XOUT_H 0x3B
#define DEV_ADDR 0x68
#define ESPNOW_WIFI_CHANNEL 6

// Receiver MAC address (slave)
uint8_t slaveMAC[6] = {0x44, 0x1D, 0x64, 0xF4, 0xAC, 0xB4}; 

class ESP_NOW_Unicast_Peer : public ESP_NOW_Peer {
public:
    ESP_NOW_Unicast_Peer(const uint8_t *mac_addr, uint8_t channel, wifi_interface_t iface, const uint8_t *lmk)
        : ESP_NOW_Peer(mac_addr, channel, iface, lmk) {}
    ~ESP_NOW_Unicast_Peer() { remove(); }

    bool begin() {
        if (!ESP_NOW.begin() || !add()) {
            log_e("Failed to initialize or register peer");
            return false;
        }
        return true;
    }

    bool send_message(const uint8_t *data, size_t len) { return send(data, len); }
};

// Create the slave peer
ESP_NOW_Unicast_Peer slavePeer(slaveMAC, ESPNOW_WIFI_CHANNEL, WIFI_IF_STA, nullptr);

void setup() {
    Serial.begin(115200);
    Wire.begin(21, 22);  // SDA, SCL

    // Wake up MPU6050
    Wire.beginTransmission(DEV_ADDR);
    Wire.write(0x6B); // PWR_MGMT_1
    Wire.write(0x00); // wake up
    Wire.endTransmission();
    delay(100);

    WiFi.mode(WIFI_STA);
    WiFi.setChannel(ESPNOW_WIFI_CHANNEL);

    if (!slavePeer.begin()) {
        Serial.println("Failed to initialize slave peer");
        delay(5000);
        ESP.restart();
    }

    Serial.println("Setup complete. Ready to send accelerometer data.");
}

void loop() {


    // Request 6 bytes: X/Y/Z
    Wire.beginTransmission(DEV_ADDR);
    Wire.write(ACCEL_XOUT_H);
    Wire.endTransmission();
    delay(2);

    uint8_t bytesReceived = Wire.requestFrom(DEV_ADDR, 6);
    
    if (bytesReceived == 6) {
        uint8_t temp[6];
        Wire.readBytes(temp, 6);

        int16_t accelX = (int16_t)((temp[0] << 8) | temp[1]);
        int16_t accelY = (int16_t)((temp[2] << 8) | temp[3]);
        int16_t accelZ = (int16_t)((temp[4] << 8) | temp[5]);

        float ax = accelX / 16384.0;
        float ay = accelY / 16384.0;
        float az = accelZ / 16384.0;

        char data[64];
        int len = snprintf(data, sizeof(data), "Accel [g]: X=%.3f, Y=%.3f, Z=%.3f", ax, ay, az);

        if (slavePeer.send_message((uint8_t*)data, len)) {
            Serial.printf("Sent: %s\n", data);
        } else {
            Serial.println("Failed to send message");
        }
    } else {
        Serial.println("No data received from MPU6050");
    }

    delay(5000);
}
