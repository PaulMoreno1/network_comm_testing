#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
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

const char index_html[] PROGMEM = R"rawliteral(
  <!DOCTYPE HTML>
  <html>
  <head>
    <title>AIOS Index</title>
  </head>
  <body>
  </body>
  </html>)rawliteral";

const uint spiCLK = 4000000;

#define NULL_CHAR "NULL"
#define NULL_STR_IP "0.0.0.0"
IPAddress nullIP = (0, 0, 0, 0);

AsyncWebServer server(80);

const char* confTemplate = "SSID:\nPASSWORD:\nLOCALIP:\nGATEWAY:\nDNS:\nSUBNET:\n";
const char* gatewayInfo =  "IPADRS:\nMACADRSI:\nMACADRSB:\nDEVICEID:\nTHIS?:\n";
const char* confFileDir = "/config3.txt";

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

enum CONNECTION_MODE
{
  NO_CREDENTIALS = 0x01,
  NO_ADDRESSES,
  NO_NETWORK,
  CONNECTED
};

enum INTERNET_MODE
{
  failedConn = 0x00,
  mAccessPoint = 0x01,
  mWiFi = 0x02,
  mStation = 0x03,
};

INTERNET_MODE setUp_Internet();
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
const char* readFile(fs::FS &fs, const char* path);
void writeFile(fs::FS &fs, const char* path, const char* message);
void appendFile(fs::FS &fs, const char* path, const char* message);
void appendInLine(fs::FS &fs, const char* path, const char* key, const char* value);

void serverOn(String htmlFile);
void htmlSetUp();
void wifiManagerVar();

const char* getValue(const char* config, const char* key);

int connectionState = 0;
INTERNET_MODE internetState;

unsigned long prevMillis = 0;
const long intervalConnWiFi = 10*1000;

void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);
  delay(5000);
  if(setUpSDCard())
  {
    currentNetwork = readConfFile();
    Serial.printf("SSID: %s - l: %i\nPASSWORD: %s - l %i\n", currentNetwork.ssid, strlen(currentNetwork.ssid), currentNetwork.password, strlen(currentNetwork.password));
    Serial.print("IP: ");
    Serial.println(currentNetwork.ipAddress);
  }

  htmlSetUp();
}

void loop()
{

}

void writeFile(fs::FS &fs, const char* path, const char* message)
{
  Serial.printf("Writing file: %s\n", path);
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

void appendInLine(fs::FS &fs, const char* path, const char* key, const char* value)
{
  String content = readFile(fs, path);
  int index = content.indexOf(key);
  if (index == -1) 
  {
    Serial.printf("Key %s not found in file.\n", key);
    return;
  }

  int valueStartIndex = content.indexOf(':', index) + 1;
  int valueEndIndex = content.indexOf('\n', valueStartIndex);
  if (valueEndIndex == -1)
  {
    valueEndIndex = content.length();
  }

  content = content.substring(0, valueStartIndex) + value + content.substring(valueEndIndex);
  writeFile(fs, path, content.c_str());
}

const char* readFile(fs::FS &fs, const char* path)
{
  Serial.printf("Reading file: %s\n", path);
  String fileString;
  File file = fs.open(path);
  if(!file)
  {
    file.close();
    Serial.printf("No configuration file found, creating one...\n");
    writeFile(fs, path, confTemplate);
    file = fs.open(path, FILE_READ, false);
  }

  while(file.available())
  {
    fileString += (char)file.read();
  }
  file.close();
  return fileString.c_str();
}

netParam readConfFile()
{
  netParam tmpParam;
  const char* tmpConf;
  tmpConf = readFile(SD, confFileDir);

  const char* ssidStr = getValue(tmpConf, "SSID:");
  if(ssidStr != NULL && strlen(ssidStr)>8)
  {
    Serial.println("SSID found!");
    connectionState = NO_ADDRESSES;
    Serial.printf("ssidStr: %s\tlen %i\n", ssidStr, strlen(ssidStr));
    tmpParam.ssid = new char[strlen(ssidStr)+1];
    
    memcpy(tmpParam.ssid, ssidStr, strlen(ssidStr));
    tmpParam.ssid[strlen(ssidStr)] = '\0';
    Serial.printf("tmpSsid: %s\tlen %i\n", tmpParam.ssid, strlen(tmpParam.ssid));
  }
  else
  {
    Serial.println("SSID not found!");
    connectionState = NO_CREDENTIALS;
    tmpParam.ssid = new char[strlen(NULL_CHAR)];
    memcpy(tmpParam.ssid, NULL_CHAR, strlen(NULL_CHAR));
  }
  
  const char* passwordStr = getValue(tmpConf, "PASSWORD:");
  if(passwordStr != NULL && strlen(passwordStr)>2)
  {
    // Serial.println("PASSWORD found!");
    tmpParam.password = new char[strlen(passwordStr) + 1];
    memcpy(tmpParam.password, passwordStr, strlen(passwordStr));
    tmpParam.password[strlen(passwordStr)] = '\0';
  }
  else
  {
    // Serial.println("PASSWORD not found!");
    tmpParam.password = new char[strlen(NULL_CHAR)];
    memcpy(tmpParam.password, NULL_CHAR, strlen(NULL_CHAR));
  }
  
  const char* ipStr = getValue(tmpConf, "LOCALIP:");
  if(ipStr != NULL && strlen(ipStr)>2)
  {
    // Serial.println("LOCAL IP found!");
    if(connectionState != NO_CREDENTIALS)
    {
      connectionState = NO_NETWORK;
    }
    tmpParam.ipAddress.fromString(ipStr);
  }
  else
  {
    // Serial.println("LOCAL IP not found!");
    if(connectionState != NO_CREDENTIALS)
    {
      connectionState = NO_ADDRESSES;
    }
    tmpParam.ipAddress.fromString(NULL_STR_IP);
  }

  const char* gatewayStr = getValue(tmpConf, "GATEWAY:");
  if(gatewayStr != NULL && strlen(gatewayStr)>2)
  {
    // Serial.println("GATEWAY found!");
    tmpParam.gateway.fromString(gatewayStr);
  }
  else
  {
    // Serial.println("GATEWAY not found!");
    tmpParam.gateway.fromString(NULL_STR_IP);
  }

  const char* dnsStr = getValue(tmpConf, "DNS:");
  if(dnsStr != NULL && strlen(dnsStr)>2)
  {
    // Serial.println("DNS found!");
    tmpParam.dns.fromString(dnsStr);
  }
  else
  {
    // Serial.println("DNS not found!");
    tmpParam.dns.fromString(NULL_STR_IP);
  }

  const char* subnetStr = getValue(tmpConf, "SUBNET:");
  if(subnetStr != NULL && strlen(subnetStr)>2)
  {
    // Serial.println("SUBNET found!");
    tmpParam.subnet.fromString(subnetStr);
  }
  else
  {
    // Serial.println("SUBNET not found!");
    tmpParam.subnet.fromString(NULL_STR_IP);
  }

  Serial.printf("ConnSta: %i\n", connectionState);
  return tmpParam;
}



bool setUpSDCard()
{
  SPIClass* spiMod;
  spiMod = new SPIClass(VSPI);
  spiMod->begin(CLK_PIN, MISO_PIN, MOSI_PIN, CS_PIN);
  spiMod->setDataMode(SPI_MODE0);
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

INTERNET_MODE setUp_Internet()
{
  //there three cases:
  //Connect to known Wi-Fi network
  //Create network
  //Connect to unknown Wi-Fi network
  INTERNET_MODE tmpMode;
  if(connectionState != NO_CREDENTIALS)
  {
    if(connectionState != NO_ADDRESSES)
    {
      WiFi.config(currentNetwork.ipAddress, currentNetwork.gateway, currentNetwork.subnet, currentNetwork.dns);
      WiFi.mode(WIFI_AP_STA);
      tmpMode = mStation;
    }
    else
    {
      WiFi.mode(WIFI_STA);
      tmpMode = mWiFi;
    }

    WiFi.begin(currentNetwork.ssid, currentNetwork.password);
    Serial.print("Connecting to WiFi...");
    unsigned long currentMillis = millis();
    prevMillis = currentMillis;
    while(WiFi.status() != WL_CONNECTED)
    {
      currentMillis = millis();
      Serial.print(".");
      if(currentMillis - prevMillis >= intervalConnWiFi)
      {
        Serial.println("\nFailed to connect");
        connectionState = NO_NETWORK;
        return failedConn;
      }
      delay(100);
    }

    Serial.print("Device IP: ");
    Serial.println(WiFi.localIP());
    return tmpMode;
    //means there is an identified network - client mode, it will get the network data and then create its own network
    //if(networkParaM) IF NETWORK PARAMETER IS EMPTY, THEN GET DATA
  }

  else
  {
    Serial.printf("No registered network, setting device as Access Point\n");
    const char* AP_SSID = "AIOS Access Point";
    const char* AP_password = "AIOS2024";
    WiFi.softAP(AP_SSID, AP_password);
    WiFi.softAPsetHostname("Gateway");
    Serial.print("AP IP Address: ");
    Serial.println(WiFi.softAPIP());
    return mAccessPoint;
    //means there is no identified network - default mode, it will request to set your network, then create a network
  }
}

const char* getValue(const char* config, const char* key) 
{
  const char *start = strstr(config, key);
  if (start != NULL) 
  {
    start += strlen(key);
    const char *end = strchr(start, '\n');
    if (end != NULL) 
    {
      size_t length = end - start;
      char *value = (char *)malloc(length + 1);
      strncpy(value, start, length);
      value[length] = '\0';
      return value;
    }
  }
  return NULL;
}

void serverOn(String htmlFile)
{
  server.on("/", HTTP_GET, [htmlFile](AsyncWebServerRequest *request){
      request->send(SD, htmlFile, "text/html");
  });
  server.serveStatic("/", SD, "/");
}

void htmlSetUp()
{
  INTERNET_MODE currMode = setUp_Internet();
  delay(1000);

  switch (currMode)
  {
    case failedConn:
      Serial.println("FAILED CONNECTION");
    break;
    case mAccessPoint:
      Serial.println("ACCESS POINT");
      serverOn("/wifiManager.html");
      wifiManagerVar();
    break;

    case mWiFi:
      Serial.println("WIFI MODE");
    break;
    case mStation:
      Serial.println("STATION MODE");
    break;

    default:
    break;
  }
  server.begin();
}

void wifiManagerVar()
{
  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request){
    int params = request->params();
    String newSSID;
    for(int i = 0; i < params; i++)
    {
      AsyncWebParameter* p = request->getParam(i);
      if (p->isPost())
      {
        if (p->name() == "ssid")
        {
          newSSID = p->value().c_str();
          Serial.printf("New SSID: %s\n", newSSID);
          appendInLine(SD, confFileDir, "SSID:", newSSID.c_str());
        }

        if (p->name() == "pass")
        {
          String newPASSWORD = p->value().c_str();
          Serial.printf("New PASSWORD: %s\n", newPASSWORD);
          appendInLine(SD, confFileDir, "PASSWORD:", newPASSWORD.c_str());
        }

        if (p->name() == "ip")
        {
          String newLOCALIP = p->value().c_str();
          Serial.printf("New LOCALIP: %s\n", newLOCALIP);
          appendInLine(SD, confFileDir, "LOCALIP:", newLOCALIP.c_str());
        }

        if (p->name() == "gateway")
        {
          String newGATEWAY = p->value().c_str();
          Serial.printf("New GATEWAY: %s\n", newGATEWAY);
          appendInLine(SD, confFileDir, "GATEWAY:", newGATEWAY.c_str());
        }

        if (p->name() == "subnet")
        {
          String newSUBNET = p->value().c_str();
          Serial.printf("New SUBNET: %s\n", newSUBNET);
          appendInLine(SD, confFileDir, "SUBNET:", newSUBNET.c_str());
        }

        if (p->name() == "dns")
        {
          String newDNS = p->value().c_str();
          Serial.printf("New DNS: %s\n", newDNS);
          appendInLine(SD, confFileDir, "DNS:", newDNS.c_str());
        }
      }
    }

    request->send(200, "text/plain", "Data registered. Device will reboot and attempt to connect to: " + newSSID);
    delay(1000);
    ESP.restart();
  });
}