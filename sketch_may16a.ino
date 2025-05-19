#include <M5Stack.h>
#include <BLEDevice.h>
#include <esp_task_wdt.h>
#include <math.h>
#define debug 0
int scanTime = 2;  //In seconds
static BLEScan* pBLEScan;

static BLEAddress* pServerAddress;
static boolean doConnect = false;
static boolean doConnectAlc = false;
static BLEClient* pClient = nullptr;

static BLERemoteService* pRemoteService;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLERemoteCharacteristic* pRemoteCharacteristicWrite;
static BLERemoteDescriptor* pBLERemoteDescriptor;
static BLEUUID serviceUUID("00001809-0000-1000-8000-00805f9b34fb");
static BLEUUID serviceAlcUUID("0000b000-0000-1000-8000-00805f9b34fb");
static BLEUUID charUUID("00002a1c-0000-1000-8000-00805f9b34fb");
static BLEUUID charAlcUUID("0000b001-0000-1000-8000-00805f9b34fb");
static BLEUUID charAlc2UUID("0000b002-0000-1000-8000-00805f9b34fb");
static BLEUUID discUUID("00002902-0000-1000-8000-00805f9b34fb");
const uint8_t indicateOn[] = { 0x2, 0x0 };
const uint8_t NotifyOn[] = { 0x1, 0x0 };
// const uint8_t* writeVal[] = *{ 0x01 };
static BLEAdvertisedDeviceCallbacks* pAdvertisedDeviceCallbacks;
static BLEDevice* pBLEDevice;
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (String("NT-100B").equals(advertisedDevice.getName().c_str())) {
      Serial.printf("{'event':'deviceFound','name':'%s','rssi':%d}\n", advertisedDevice.getName().c_str(), advertisedDevice.getRSSI());
      advertisedDevice.getScan()->stop();
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      doConnect = true;
    } else if (String("HCS-AC01").equals(advertisedDevice.getName().c_str())) {
      Serial.printf("{'event':'deviceFound','name':'%s','rssi':%d}\n", advertisedDevice.getName().c_str(), advertisedDevice.getRSSI());
      doConnect = true;
      doConnectAlc = true;

      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
    } else {
    }
  }
};


static void notifyCallbackAlc(
  BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  // if (debug) Serial.println("V");
  // for (int i = 0; i <= length - 1; i++) {
  //   if (debug) Serial.printf("%X ", pData[i]);
  // }
  if (debug) Serial.println();
  int foo;
  foo = (int)pData[5] << 24;
  foo |= (int)pData[4] << 16;
  foo |= (int)pData[3] << 8;
  foo |= (int)pData[2];
  // //https://forum.arduino.cc/t/convert-byte-array-to-uint32_t/116783/4
  int mantissa = foo & 0xffffff;
  int exponential = foo >> 24;
  if (debug) Serial.printf("{'0':%x,'1':%x,'mantissa':%d,'exponential':%d}\n", pData[0], pData[1], mantissa, exponential);
  switch (pData[0]) {
    case 0:
      doConnectAlc = false;
      break;
    case 1:

      Serial.printf("{'event':'initialize','value':%d}\n", mantissa);
      break;
    case 2:

      Serial.printf("{'event':'blowing','value':%d}\n", mantissa);
      break;
    // pRemoteCharacteristicWrite->writeValue({ 0x01 }, true);
    case 3:
      Serial.printf("{'event':'blowfin'}\n");
      break;
    case 4:
      float f;
      uint8_t rxBuffer[4];
      //https://gregstoll.dyndns.org/~gregstoll/floattohex/

      rxBuffer[0] = pData[2];  // The first byte of data.
      rxBuffer[1] = pData[3];  // Second byte of data.
      rxBuffer[2] = pData[4];  // etc.
      rxBuffer[3] = pData[5];
      f = *((float*)&rxBuffer);

      Serial.printf("{'event':'alcCalfin',value:%f}\n", f);
      //https://forum.arduino.cc/t/transform-4-bytes-into-a-float/599241/4
      // doConnectAlc = false;
      break;
  }

  // Serial.printf("{'event':'temp','val':%f}\n", mantissa * pow(10, exponential));
  // pClient->disconnect();
}


static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  // if (debug) Serial.println("V");
  // for (int i = 0; i <= length - 1; i++) {
  //   if (debug) Serial.printf("%X ", pData[i]);
  // }
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
  // pClient->disconnect();
}


class funcClientCallbacks : public BLEClientCallbacks {
  void onConnect(BLEClient* pClient){};
  void onDisconnect(BLEClient* pbClient) {
    if (pbClient->isConnected()) {
      Serial.print("lls");
      pbClient->disconnect();
    }
    doConnect = false;
    doConnectAlc = false;
    Serial.println("disconnect");
  }
};




bool connectToAlcServer(BLEAddress* pAddress) {

  if (!pClient->connect(*pAddress)) {

    pClient->disconnect();
    return false;
  }
  if (debug) Serial.println("connect");
  pRemoteService = pClient->getService(serviceAlcUUID);

  if (debug) Serial.print("S");
  if (pRemoteService == nullptr) {
    if (debug) Serial.println("pRemoteService null");
    return false;
  }

  pRemoteCharacteristic = pRemoteService->getCharacteristic(charAlcUUID);
  if (debug) Serial.println("C");
  if (pRemoteCharacteristic == nullptr) {

    if (debug) Serial.println("pRemoteCharacteristic null");
    return false;
  }

  if (pRemoteCharacteristic->canRead()) {
    if (String("").equals(pRemoteCharacteristic->readValue().c_str())) {
      {
        pRemoteCharacteristicWrite = pRemoteService->getCharacteristic(charAlc2UUID);
        Serial.printf("write %s\n", pRemoteCharacteristicWrite->toString().c_str());

        pRemoteCharacteristicWrite->writeValue({ 0x01 }, true);
        // pRemoteCharacteristicWrite->get
        // pRemoteCharacteristicWrite->writeValue(writeVal, 1, true);
        // pRemoteCharacteristicWrite->
        Serial.println("write value 0x01");
        Serial.printf("read value:%s\n", pRemoteCharacteristicWrite->readValue().c_str());

        // if (debug) Serial.printf("%s\r\n", pRemoteCharacteristic->toString().c_str());
        pBLERemoteDescriptor = pRemoteCharacteristic->getDescriptor(discUUID);
        if (debug) Serial.printf("%s\r\n", pBLERemoteDescriptor->toString().c_str());
        if (debug) Serial.println("N");

        if (debug) Serial.printf("pRemoteCharacteristic: %s\n", pRemoteCharacteristic->toString().c_str());
        pRemoteCharacteristic->registerForNotify(notifyCallbackAlc, true, true);
        if (debug) Serial.println("N2");
        // delay(8000);

        if (debug) Serial.print("D");
        pBLERemoteDescriptor->writeValue((uint8_t*)NotifyOn, 2, true);
        esp_task_wdt_reset();
      }
    }
  }

  if (debug) Serial.println("W");
  return true;
}




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
  pClient->setClientCallbacks(new funcClientCallbacks());

  bleInit();
  esp_task_wdt_init(15, true);
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
    if (doConnectAlc == true) {
      if (debug) Serial.println("connectiong alc...");
      connectToAlcServer(pServerAddress);
      while (doConnectAlc) {

        esp_task_wdt_reset();
        delay(100);
      }
      pClient->disconnect();
      doConnectAlc = false;


    } else {
      if (connectToServer(pServerAddress)) {
        delay(6000);
        pClient->disconnect();
      } else {
      }
    }
    free(pServerAddress);
    doConnect = false;
  }
  pBLEScan->clearResults();
  if (debug) Serial.printf("\r\nMALLOC_CAP_INTERNAL:%d\r", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
  if (debug) Serial.print(".");
  delay(100);
}
