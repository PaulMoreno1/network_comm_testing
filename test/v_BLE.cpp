#ifdef SERVER
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <BluetoothSerial.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define SERIAL_BAUD_RATE    115200

                            //41494F53 (AIOS)
                            //7376 (sv - service) - 6368 (ch - characteristic) - 7270 (rp - response)
                            //3234 is 24 
                            //7e4b -> ~K
                            //2a4f -> *O
                            //304431366f7e -> 0D16o~
                            //6b553154302a -> kU1T0*
#define SERVICE_UUID        "41494f53-7376-3234-74be-7304431366fe"
#define CHARACTERISTIC_UUID "41494f53-6368-3234-24fa-26b55315430a"
#define RESPONDE_UUID       "41494f53-7270-3234-74fe-76b55315430e"

BLECharacteristic *pCharacteristic;
BLECharacteristic *pResponseCharacteristic;

class MyCallbacks : public BLECharacteristicCallbacks 
{
  void onWrite(BLECharacteristic *pCharacteristic) 
  {
    std::string value = pCharacteristic->getValue();
    if (value.length() > 0) {
      Serial.print("Received Value: ");
      for (int i = 0; i < value.length(); i++)
        Serial.print(value[i]);

      Serial.println();

      pResponseCharacteristic->setValue("OK");
      pResponseCharacteristic->notify();
    }
  }
};

void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.printf("BLE Server\n");
  BLEDevice::init("ESP32 - AIOS BT");
  BLEServer   *pServer = BLEDevice::createServer();
  BLEService  *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );

  pResponseCharacteristic = pService->createCharacteristic(
    RESPONDE_UUID, BLECharacteristic::PROPERTY_NOTIFY
  );
  
  pCharacteristic->setCallbacks(new MyCallbacks());

  pCharacteristic->setValue("BLE from ESP32 BT Server");
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  Serial.printf("Characteristic defined - ready to read it in the phone\n");
}

void loop()
{
  delay(10);
}

#elif CLIENT

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLEAdvertisedDevice.h>

#define SERVICE_UUID        "41494f53-7376-3234-74be-7304431366fe"
#define CHARACTERISTIC_UUID "41494f53-6368-3234-24fa-26b55315430a"
#define RESPONDE_UUID       "41494f53-7270-3234-74fe-76b55315430e"

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

#endif