#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLEAdvertisedDevice.h>

#define SERVICE_UUID        "41494f53-7376-3234-74be-7304431366fe"
#define CHARACTERISTIC_UUID "41494f53-6368-3234-24fa-26b55315430a"

BLEScan* pBLEScan;
bool deviceFound = false;
BLEAdvertisedDevice* myDevice;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks 
{
  void onResult(BLEAdvertisedDevice advertisedDevice) 
  {
    Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID(SERVICE_UUID))) 
    {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      deviceFound = true;
    }
  }
};

void setup() 
{
  Serial.begin(115200);
  BLEDevice::init("ESP32_BLE_CLIENT");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(30);
}

void loop() 
{
  if (deviceFound) 
  {
    deviceFound = false;
    BLEClient* pClient = BLEDevice::createClient();
    pClient->connect(myDevice);
    Serial.println("Connected to server");

    BLERemoteService* pRemoteService = pClient->getService(BLEUUID(SERVICE_UUID));
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(SERVICE_UUID);
      pClient->disconnect();
      return;
    }

    BLERemoteCharacteristic* pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(CHARACTERISTIC_UUID));
    if (pRemoteCharacteristic == nullptr) 
    {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(CHARACTERISTIC_UUID);
      pClient->disconnect();
      return;
    }

    if(pRemoteCharacteristic->canRead()) 
    {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }

    pClient->disconnect();
    Serial.println("Disconnected from server");
  }
}
