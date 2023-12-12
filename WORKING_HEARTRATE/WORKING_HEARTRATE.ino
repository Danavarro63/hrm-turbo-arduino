#include "BLEDevice.h"

// UUIDs for Heart Rate Monitor
static BLEUUID serviceUUID_HRM("0000180d-0000-1000-8000-00805f9b34fb"); // Heart Rate Service UUID
static BLEUUID charUUID_HRM(BLEUUID((uint16_t)0x2A37)); // Heart Rate Measurement Characteristic UUID

// UUIDs for Power Meter
static BLEUUID serviceUUID_Power("00001818-0000-1000-8000-00805f9b34fb"); // Power Service UUID
static BLEUUID charUUID_Power(BLEUUID((uint16_t)0x2A63)); // Power Measurement Characteristic UUID

static boolean doConnect = false;
static BLERemoteCharacteristic* pRemoteCharacteristic_HRM;
static BLERemoteCharacteristic* pRemoteCharacteristic_Power;
static BLEAdvertisedDevice* myDevice_HRM;
static BLEAdvertisedDevice* myDevice_Power;

static void notifyCallback_HRM(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  int BPM = pData[1];
  Serial.print("Heart Rate: ");
  Serial.print(BPM);
  Serial.println(" BPM");
}

static void notifyCallback_Power(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  if (length >= 4) {
    int instantPower = pData[2] | (pData[3] << 8);
    Serial.print("Instant Power: ");
    Serial.print(instantPower);
    Serial.println(" Watts");
  }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    Serial.println("Disconnected");
  }
};

bool connectToServer(BLEAdvertisedDevice* myDevice, BLERemoteCharacteristic** pRemoteCharacteristic, BLEUUID serviceUUID, BLEUUID charUUID, void (*notifyCallback)(BLERemoteCharacteristic*, uint8_t*, size_t, bool)) {
  Serial.print("Connecting to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient* pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  pClient->connect(myDevice);

  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.println("Failed to find service UUID");
    pClient->disconnect();
    return false;
  }

  *pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (*pRemoteCharacteristic == nullptr) {
    Serial.println("Failed to find characteristic UUID");
    pClient->disconnect();
    return false;
  }

  if ((*pRemoteCharacteristic)->canNotify()) {
    (*pRemoteCharacteristic)->registerForNotify(notifyCallback);
  }

  return true;
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID_HRM)) {
      Serial.println("Found Heart Rate Monitor");
      myDevice_HRM = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    } else if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID_Power)) {
      Serial.println("Found Power Meter");
      myDevice_Power = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE Client");
  BLEDevice::init("");

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30, false);
}

void loop() {
  if (doConnect) {
    if (myDevice_HRM != nullptr) {
      connectToServer(myDevice_HRM, &pRemoteCharacteristic_HRM, serviceUUID_HRM, charUUID_HRM, notifyCallback_HRM);
    }
    if (myDevice_Power != nullptr) {
      connectToServer(myDevice_Power, &pRemoteCharacteristic_Power, serviceUUID_Power, charUUID_Power, notifyCallback_Power);
    }
    doConnect = false;
  }

  delay(1000);
}
