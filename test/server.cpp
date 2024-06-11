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
                            //7376 (sv - service) - 6368 (ch - characteristic)
                            //3234 is 24 
                            //7e4b -> ~K
                            //2a4f -> *O
                            //304431366f7e -> 0D16o~
                            //6b553154302a -> kU1T0*
#define SERVICE_UUID        "41494f53-7376-3234-74be-7304431366fe"
#define CHARACTERISTIC_UUID "41494f53-6368-3234-24fa-26b55315430a"

BLECharacteristic *pCharacteristic;

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

  delay(1000);
}
