#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "87654321-4321-4321-4321-0987654321ba"

BLEServer* pServer = nullptr;
BLECharacteristic* pCharacteristic = nullptr;
bool deviceConnected = false;

// Callback class for handling connections
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Central connected!");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Central disconnected!");
    pServer->getAdvertising()->start(); // Restart advertising
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work...");

  // Initialize BLE
  BLEDevice::init("TipUp_Alert"); // Device name visible to iPhone

  // Create BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create BLE Service
  BLEService* pService = pServer->createService(SERVICE_UUID);

  // Create BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  // Initial value
  pCharacteristic->setValue("Waiting...");

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // Helps with iPhone compatibility
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.println("BLE advertising started!");
}

void loop() {
  static unsigned long lastUpdate = 0;

  // Every 10 seconds, send a simulated alert
  if (millis() - lastUpdate > 10000) {
    lastUpdate = millis();

    if (deviceConnected) {
      String alertMsg = "TIP-UP ALERT!";
      pCharacteristic->setValue(alertMsg.c_str());
      pCharacteristic->notify(); // Send notification to iPhone app
      Serial.println("Sent notification: " + alertMsg);
    } else {
      Serial.println("No central connected yet...");
    }
  }
}
