/**
 * A BLE client that connectes to and streams live power data from a tacx flux
 * Daniel's awesome work below :)
 */

#include "BLEDevice.h"

// The remote service we wish to connect to.
static BLEUUID serviceUUID("00001818-0000-1000-8000-00805f9b34fb");  //tacx flux
//serviceUUID: 00001826-0000-1000-8000-00805f9b34fb, serviceUUID: 00001818-0000-1000-8000-00805f9b34fb(instant power), serviceUUID: 669aa605-0c08-969e-e211-86ad5062675f
// The characteristic of the remote service we are interested in.
static BLEUUID charUUID(BLEUUID((uint16_t)0x2A63));

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
static std::string connectedDeviceAddress = "ff:93:31:e0:f4:fc";  // UPDATE to tacx flux
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  if (length >= 4) {
    // Bytes 2 and 3 represent Instant Power (16-bit little-endian)
    int instantPower = pData[2] | (pData[3] << 8);
    Serial.print("Instant Power: ");
    Serial.println(instantPower);
    Serial.println("Watts");
  }
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient* pClient = BLEDevice::createClient();

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remove BLE Server.
  pClient->connect(myDevice);  // If you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)

  // Store the address of the connected device
  if (pClient->isConnected()) {
    connectedDeviceAddress = myDevice->getAddress().toString();
    connected = true;
  } else {
    connected = false;
    return false;
  }

  pClient->setMTU(517);  // Set client to request maximum MTU from server (default is 23 otherwise)

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }

  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }

  // Read the value of the characteristic.
  if (pRemoteCharacteristic->canRead()) {
    std::string value = pRemoteCharacteristic->readValue();
  }

  if (pRemoteCharacteristic->canNotify())
    pRemoteCharacteristic->registerForNotify(notifyCallback);

  return true;
}

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      Serial.print("BLE Advertised Device found: ");
      Serial.println(advertisedDevice.toString().c_str());
      Serial.println("Found Power Service");
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = false;
    } else {

    }  // Found our server
  }    // onResult
};     // MyAdvertisedDeviceCallbacks
bool isConnectedToSpecificDevice(const std::string& specificDeviceAddress) {
  return connected && (connectedDeviceAddress == specificDeviceAddress);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting the Power Program");
  BLEDevice::init("");


  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(25, false);
}



void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    // Set the characteristic's value to be the array of bytes that is actually a string.
  } else if (doScan) {
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }

  delay(500);  // Delay a second between loops.
}  // End of loop
