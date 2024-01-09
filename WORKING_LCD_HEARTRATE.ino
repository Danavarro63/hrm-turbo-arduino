/**
 * A BLE client that connectes to and streams live heartrate data from a wahoo tickr
 * Daniel's awesome work below :)
 */

#include "BLEDevice.h"
#include <Adafruit_NeoPixel.h>
#include <U8x8lib.h>

#define LED_PIN A3
#define LED_COUNT 150
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
U8X8_SSD1306_128X32_UNIVISION_HW_I2C u8x8(U8X8_PIN_NONE);

const int bright = 20;
volatile int BPM;



// The remote service we wish to connect to.
static BLEUUID serviceUUID("0000180d-0000-1000-8000-00805f9b34fb");
// The characteristic of the remote service we are interested in.
static BLEUUID charUUID(BLEUUID((uint16_t)0x2A37));

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
static std::string connectedDeviceAddress = "ef:e2:0c:b6:4b:a8";
static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  BPM = pData[1];
  int lastHR;
  int currentHR;
  currentHR = BPM;
  // Format the heart rate value with leading zeros
  char formattedBPM[4];
  sprintf(formattedBPM, "%03d", BPM);

  Serial.print("Heart Rate :");
  Serial.print(BPM);
  Serial.println(" BPM");
  u8x8.setFont(u8x8_font_inr21_2x4_n);
  u8x8.setCursor(3, 0);
  u8x8.print(formattedBPM);  // Display formatted heart rate
  u8x8.setFont(u8x8_font_torussansbold8_r);
  u8x8.setCursor(10, 2);
  u8x8.print("BPM");

  lastHR = currentHR;
  // Read the BPM value and update LED color based on heart rate range
  int heartRate = BPM;
  int pause = 100;
  uint32_t color = 0;  // Default color (off)
  // Choose LED color based on heart rate range
  Serial.println("checking zone");
  if (heartRate == 0) {
    color = strip.Color(0, 0, 0);
  } else if (heartRate > 0 && heartRate < 50) {
    color = strip.Color(255, 255, 255);  // White  // White, half brightness
  } else if (heartRate >= 50 && heartRate < 60) {
    color = strip.Color(0, 0, 255);  // Blue  // Blue, half brightness
  } else if (heartRate >= 60 && heartRate < 70) {
    color = strip.Color(0, 255, 0);  // Green, half brightness
  } else if (heartRate >= 70 && heartRate < 80) {
    color = strip.Color(255, 255, 0);  // Yellow, half brightness
  } else if (heartRate >= 80 && heartRate < 90) {
    color = strip.Color(255, 165, 0);  // Orange, half brightness
  } else if (heartRate >= 90) {
    color = strip.Color(255, 0, 0);  // Red, half brightness
  }
  // Set all pixels to the chosen color
  for (int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, color);
  }
  strip.show();  // Update strip with new contents
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
    u8x8.clear();
    u8x8.setFont(u8x8_font_torussansbold8_r);
    u8x8.setCursor(0, 0);
    u8x8.print("Heartrate Lost!");
    u8x8.setCursor(0, 1);
    u8x8.print("Restart this");
    u8x8.setCursor(0, 2);
    u8x8.print("device to");
    u8x8.setCursor(0, 3);
    u8x8.print("reconnect.");
  }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());
  u8x8.setFont(u8x8_font_torussansbold8_r);
  u8x8.setCursor(0, 1);
  u8x8.print("Connecting to:");
  u8x8.setFont(u8x8_font_5x8_r);
  u8x8.setCursor(0, 2);
  u8x8.print(myDevice->getAddress().toString().c_str());
  delay(500);

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
      Serial.println("Found Heart Rate Service");
      u8x8.setFont(u8x8_font_torussansbold8_r);
      u8x8.setCursor(0, 1);
      u8x8.print("Found Service");
      delay(500);

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
  u8x8.begin();

  Serial.println("Starting the Heart rate Program");
  u8x8.setFont(u8x8_font_torussansbold8_r);
  u8x8.setCursor(0, 1);
  u8x8.print("Starting Scan");
  delay(500);

  strip.begin();                // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();                 // Turn OFF all pixels ASAP
  strip.setBrightness(bright);  // Set BRIGHTNESS to about 1/5 (max = 255)

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
      u8x8.clear();
      u8x8.setFont(u8x8_font_torussansbold8_r);
      u8x8.setCursor(0, 1);
      u8x8.print("Connected");
      delay(500);
      u8x8.clearDisplay();
    } else {
      Serial.println("We have failed to connect to the server; there is nothing more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    // Set the characteristic's value to be the array of bytes that is actually a string.
  } else if (doScan) {
    BLEDevice::getScan()->start(0);  // this is just an example to start scan after disconnect, most likely there is a better way to do it in Arduino
  }
}  // End of loop
