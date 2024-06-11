#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#define CLK_PIN   18
#define MISO_PIN  19
#define MOSI_PIN  23
#define CS_PIN     5

const uint spiCLK = 4000000;

void readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while(file.available()){
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\n", path);
  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if(file.print(message)){
      Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

void sdManagement()
{
  SPIClass* spiMod;
  spiMod = new SPIClass(VSPI);
  Serial.printf("spiMod created\n");
  spiMod->begin(CLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
  Serial.printf("Pins defined\n");
  spiMod->setDataMode(SPI_MODE0);

  if(!SD.begin(CS_PIN, *spiMod, spiCLK))
  {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
    Serial.println("MMC");
  } else if(cardType == CARD_SD){
    Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
}

void otherStuff()
{
  writeFile(SD, "/CHINGAO.txt", "Not ");
  appendFile(SD, "/CHINGAO.txt", "Using\n");
  readFile(SD, "/CHINGAO.txt");
}

void setup(){
  Serial.begin(115200);
  sdManagement();
  otherStuff();
}

void loop(){

}