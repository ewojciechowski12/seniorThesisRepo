#include <NimBLEDevice.h>
#include <WiFi.h>
#include <esp_now.h>

#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-0987654321ba"
#define ESPNOW_CHANNEL 6


NimBLEServer* pServer = nullptr;
NimBLECharacteristic* pCharacteristic = nullptr;
NimBLEAdvertising* pAdvertising = nullptr;
bool BLEDeviceConnected = false;


// Callback class for handling connections
class MyServerCallbacks : public NimBLEServerCallbacks {
public:
  // Must match NimBLEServerCallbacks exactly:
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    BLEDeviceConnected = true;
    Serial.println("Central connected!");
    // keep this short — copy data to a queue if you need to process it
  }

  // Must match NimBLEServerCallbacks exactly:
  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
    BLEDeviceConnected = false;
    Serial.printf("Central disconnected (reason %d)\n", reason);

    // Restart advertising quickly and non-blocking
    NimBLEDevice::getAdvertising()->start();
  }
};

static MyServerCallbacks bleServerCallBacks;

// Callback for receiving ESP-NOW messages
void onReceive(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    const uint8_t *mac = recv_info->src_addr;
    Serial.printf("Received from %02X:%02X:%02X:%02X:%02X:%02X: %.*s\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], len, data);
}

void InitBLE(){
   Serial.println("Starting BLE work...");

  // Initialize BLE
  NimBLEDevice::init("TipUp_Alert"); // Device name visible to iPhone

  // Create BLE Server
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(&bleServerCallBacks);

  // Create BLE Service
  NimBLEService* pService = pServer->createService(SERVICE_UUID);

  // Create BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      NIMBLE_PROPERTY::READ |
                      NIMBLE_PROPERTY::NOTIFY |
                      NIMBLE_PROPERTY::WRITE
                    );


  // Initial value
  pCharacteristic->setValue("Waiting...");

  // Start the service
  pService->start();

  // Start advertising
  pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setName("TipUp_Alert");
  pAdvertising->setAppearance(0x00);
  pAdvertising->start();

  Serial.println("BLE advertising started!");

}

void InitESPNow(){
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

void sendAlert(){
   if (BLEDeviceConnected) {
      String alertMsg = "TIP-UP ALERT!";
      pCharacteristic->setValue(alertMsg.c_str());
      pCharacteristic->notify(); // Send notification to iPhone app
      Serial.println("Sent notification: " + alertMsg);
    } else {
      Serial.println("No central connected yet...");
    }
}

void setup() {
  Serial.begin(115200);

  InitBLE(); 
  InitESPNow();

 }

void loop() {
  static unsigned long lastUpdate = 0;

  sendAlert();
  delay(10000);

}
