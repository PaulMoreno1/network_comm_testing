#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <BluetoothSerial.h>
#include <WiFiUdp.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>

#define SERIAL_BAUD_RATE    115200

#define CLK_PIN   18
#define MISO_PIN  19
#define MOSI_PIN  23
#define CS_PIN     5

#define INTERNET_AP_MODE    0
#define INTERNET_WF_MODE    1
#define INTERNET_ST_MODE    2

const uint spiCLK = 4000000;

#define NULL_CHAR       "NULL"
IPAddress nullIP = (0, 0, 0, 0);

const char* confTemplate = "SSDI:\nPASSWORD:\nLOCALIP:\nGATEWAY:\nDNS:\nSUBNET:\n";
const char* gatewayInfo =  "IPADRS:\nMACADRSI:\nMACADRSB:\nDEVICEID:\nTHIS?:\n";
const char* confFileDir = "/a.txt";

struct netParam
{
  char* ssid;
  char* password;
  IPAddress ipAddress;
  IPAddress gateway;
  IPAddress dns;
  IPAddress subnet;
};

netParam currentNetwork;

enum INTERNET_MODE
{
  Access_Point_Mode,
  Wi_Fi_Mode,
  Ethernet_Mode,
  Station_Mode,
  EthernetStation_Mode
};

int setUp_Internet();
bool setUp_MQTT();
bool setUp_UDP();
bool setUp_ESPNOW();
bool setUp_Bluetooth();
bool setUp_LoRa();

void runInternet();
void runMQTT();
void runUDP();
void runESPNOW();
void runBluetooth();
void runLoRa();

bool setUpSDCard();
netParam readConfFile();
String readFile(fs::FS &fs, const char* path);
void writeFile(fs::FS &fs, const char* path, const char* message);

void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);
  if(setUpSDCard())
  {
    currentNetwork = readConfFile();
    Serial.printf("SSID: %s\tPASSWORD: %s\nIP Address %i\n", currentNetwork.ssid, currentNetwork.password, currentNetwork.ipAddress);
  }
}

void loop()
{

}

void writeFile(fs::FS &fs, const char* path, const char* message)
{
  Serial.printf("Writing file: %s\n", path);
  delay(1000);
  File file = fs.open(path, FILE_WRITE, true);
  if(!file)
  {
    Serial.print("Could not write file\n");
    return;
  }

  if(file.print(message))
  {
    Serial.printf("File written with: %s\n", message);
  }

  else
  {
    Serial.print("Could not write file\n");
  }

  file.close();
}

String readFile(fs::FS &fs, const char* path)
{
  Serial.printf("Reading file: %s\n", path);
  String fileString;
  Serial.printf(".\n");
  File file = fs.open(path);
  Serial.printf("*\n");
  delay(1000);
  Serial.printf("¨\n");
  if(!file)
  {
    Serial.printf("+\n");
    file.close();
    Serial.printf("-\n");
    Serial.printf("No configuration file found, creating one...\n");
    writeFile(fs, path, confTemplate);
    Serial.printf("_\n");
    file = fs.open(path, FILE_READ, false);
    Serial.printf("-\n");
  }

  while(file.available())
  {
    fileString += (char)file.read();
  }

  file.close();

  return fileString;
}

netParam readConfFile()
{
  netParam tmpParam;
  String tmpConf;

  tmpConf = readFile(SD, confFileDir);
  
  strcpy(tmpParam.ssid, NULL_CHAR);
  strcpy(tmpParam.password, NULL_CHAR);
  tmpParam.ipAddress = nullIP;
  tmpParam.gateway = nullIP;
  tmpParam.dns = nullIP;
  tmpParam.subnet = nullIP;

  auto getValue = [&](const String& label) -> String
  {
    int start = tmpConf.indexOf(label);
    if (start == -1)
    {
      return "";
    }
    start += label.length() + 1;
    while(start < tmpConf.length() && tmpConf[start] == ' ')
    {
      start++;
    }

    int end = tmpConf.indexOf('\n', start);
    if(end == -1)
    {
      end = tmpConf.length();
    }
    String value = tmpConf.substring(start, end);
    value.trim();
    return value;
  };

  String ssidStr = getValue("SSID:");
  if(!ssidStr.isEmpty())
  {
    strncpy(tmpParam.ssid, ssidStr.c_str(), sizeof(tmpParam.ssid) - 1);
  }

  String passwordStr = getValue("PASSWORD:");
  if(!passwordStr.isEmpty())
  {
    strncpy(tmpParam.password, passwordStr.c_str(), sizeof(tmpParam.password) - 1);
  }

  String ipStr = getValue("LOCALIP:");
  if(!ipStr.isEmpty())
  {
    tmpParam.ipAddress.fromString(ipStr);
  }

  String gatewayStr = getValue("GATEWAY:");
  if(!gatewayStr.isEmpty())
  {
    tmpParam.gateway.fromString(gatewayStr);
  }

  String dnsStr= getValue("DNS:");
  if(!dnsStr.isEmpty())
  {
    tmpParam.dns.fromString(dnsStr);
  }

  String subnet= getValue("SUBNET:");
  if(!subnet.isEmpty())
  {
    tmpParam.subnet.fromString(subnet);
  }

  return tmpParam;
}



bool setUpSDCard()
{
  SPIClass* spiMod;
  spiMod = new SPIClass(VSPI);
  Serial.printf("spiMod created\n");
  spiMod->begin(CLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
  Serial.printf("Pins defined\n");
  spiMod->setDataMode(SPI_MODE0);
  Serial.printf("Mode set\n");

  if(!SD.begin(CS_PIN, *spiMod, spiCLK))
  {
    Serial.printf("Card mount has failed!\n");
    delay(100);
    return false;
  }

  uint8_t cardType = SD.cardType();
  Serial.printf("SD Card type:\n");

  switch (cardType)
  {
    case CARD_MMC:
      Serial.printf("MMC - Multi Media Card\n");
    break;
  
    case CARD_SD:
      Serial.printf("SD - Secure Digital\n");
    break;

    case CARD_SDHC:
      Serial.printf("SDHC - Secure Digital High Capacity\n");
    break;

    case CARD_NONE:
      Serial.printf("No SD Card found!\n");
      return false;
    break;

    default:
      return false;
    break;
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card size: %i MB\n", cardSize);
  return true;
}

int setUp_Internet()
{
  //there three cases:
  //Connect to known Wi-Fi network
  //Create network
  //Connect to unknown Wi-Fi network
  if(currentNetwork.ssid != "SSID_ESP_AIOS")
  {
    WiFi.begin(currentNetwork.ssid, currentNetwork.password);
    //means there is an identified network - client mode, it will get the network data and then create its own network
    //if(networkParaM) IF NETWORK PARAMETER IS EMPTY, THEN GET DATA
  }

  else
  {
    //means there is no identified network - default mode, it will request to set your network, then create a network
  }
}