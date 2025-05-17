#include <M5Stack.h>
#include <BLEDevice.h>
#include <esp_task_wdt.h>
#include <math.h>
#define debug 0
int scanTime = 1;  //In seconds
static BLEScan* pBLEScan;

static BLEAddress* pServerAddress;
static boolean doConnect = false;
static BLEClient* pClient = nullptr;

static BLERemoteService* pRemoteService;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLERemoteDescriptor* pBLERemoteDescriptor;
static BLEUUID serviceUUID("00001809-0000-1000-8000-00805f9b34fb");
static BLEUUID charUUID("00002a1c-0000-1000-8000-00805f9b34fb");
static BLEUUID discUUID("00002902-0000-1000-8000-00805f9b34fb");
const uint8_t indicateOn[] = { 0x2, 0x0 };
static BLEAdvertisedDeviceCallbacks* pAdvertisedDeviceCallbacks;
static BLEDevice* pBLEDevice;
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (String("NT-100B").equals(advertisedDevice.getName().c_str())) {
      Serial.printf("{'event':'deviceFound','name':'%s','rssi':%d}\n", advertisedDevice.getName().c_str(), advertisedDevice.getRSSI());
      advertisedDevice.getScan()->stop();
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      doConnect = true;
    } else {
    }
  }
};


static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  if (debug) Serial.println("V");
  for (int i = 0; i <= length - 1; i++) {
    if (debug) Serial.printf("%X ", pData[i]);
  }
  if (debug) Serial.println();
  int foo;
  foo = (int)pData[4] << 24;
  foo |= (int)pData[3] << 16;
  foo |= (int)pData[2] << 8;
  foo |= (int)pData[1];
  //https://forum.arduino.cc/t/convert-byte-array-to-uint32_t/116783/4
  int mantissa = foo & 0xffffff;
  int exponential = foo >> 24;
  if (debug) Serial.printf("mantissa:%d,exponential:%d\n", mantissa, exponential);
  Serial.printf("{'event':'temp','val':%f}\n", mantissa * pow(10, exponential));
  pClient->disconnect();
}


class funcClientCallbacks : public BLEClientCallbacks {
  void onDisconnect(BLEClient* pbClient) {
    if (pbClient->isConnected()) {
      Serial.print("lls");
      pbClient->disconnect();
    }
    Serial.println("disconnect");
  }
};



bool connectToServer(BLEAddress* pAddress) {

  if (!pClient->connect(*pAddress)) {

    pClient->disconnect();
    return false;
  }
  if (debug) Serial.println("connect");
  pRemoteService = pClient->getService(serviceUUID);

  if (debug) Serial.print("S");
  if (pRemoteService == nullptr) {
    if (debug) Serial.println("pRemoteService null");
    return false;
  }

  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (debug) Serial.println("C");
  if (pRemoteCharacteristic == nullptr) {

    if (debug) Serial.println("pRemoteCharacteristic null");
    return false;
  }

  if (debug) Serial.printf("%s\r\n", pRemoteCharacteristic->toString().c_str());
  pBLERemoteDescriptor = pRemoteCharacteristic->getDescriptor(discUUID);
  if (debug) Serial.printf("%s\r\n", pBLERemoteDescriptor->toString().c_str());
  if (debug) Serial.println("N");
  esp_task_wdt_reset();

  pRemoteCharacteristic->registerForNotify(notifyCallback);
  if (debug) Serial.print("D");
  pBLERemoteDescriptor->writeValue((uint8_t*)indicateOn, 2, true);
  if (debug) Serial.println("W");
  return true;
}

void setup() {
  Serial.begin(9600);
  Serial.println("Scanning...");
  pAdvertisedDeviceCallbacks = new MyAdvertisedDeviceCallbacks();

  pClient = BLEDevice::createClient();
  bleInit();
  esp_task_wdt_init(2, true);
  esp_task_wdt_add(NULL);
}

void bleInit() {
  BLEDevice::init("");
  pBLEScan = (BLEScan*)malloc(sizeof(BLEScan));
  pBLEScan = BLEDevice::getScan();  //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(pAdvertisedDeviceCallbacks);
  pBLEScan->setActiveScan(true);  //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
}

void loop() {
  esp_task_wdt_reset();
  if (doConnect == false)
    pBLEScan->start(scanTime, false);
  delay(10);
  if (doConnect == true) {
    esp_task_wdt_reset();
    if (connectToServer(pServerAddress)) {
      pClient->disconnect();
    } else {
    }
    free(pServerAddress);
    doConnect = false;
  }
  pBLEScan->clearResults();
  if(debug)Serial.printf("\r\nMALLOC_CAP_INTERNAL:%d\r", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  if(debug)Serial.print(".");
  delay(100);
}
