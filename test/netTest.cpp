#include <Arduino.h>
#include <WiFi.h>

struct netParam
{
  char* ssid;
  char* password;
  IPAddress localIP;
  IPAddress gatewayIP;
  IPAddress dnsIP;
  IPAddress subnetMask;
};

netParam currentNetwork;

netParam getCredentials();
netParam getNetConf();

void setup()
{
  Serial.begin(115200);
  WiFi.begin("e-ON_local", "Ruben2024");
  while(WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(200);
  }

  if(WiFi.status() == WL_CONNECTED)
  {
    currentNetwork = getCredentials();
    currentNetwork = getNetConf();
    Serial.print("\nSSID: ");
    Serial.println(currentNetwork.ssid);
    Serial.print("Password: ");
    Serial.println(currentNetwork.password);
    Serial.print("Local IP: ");
    Serial.println(currentNetwork.localIP);
    Serial.print("Gateway IP: ");
    Serial.println(currentNetwork.gatewayIP);
    Serial.print("DNS IP: ");
    Serial.println(currentNetwork.dnsIP);
    Serial.print("Subnet Mask: ");
    Serial.println(currentNetwork.subnetMask);
  }
}

void loop()
{

}

netParam getCredentials()
{
  const char* ssid = "e-ON_local";
  const char* password = "Ruben2024";
  netParam tmpParam;
  tmpParam.ssid = new char[strlen(ssid)+1];
  memcpy(tmpParam.ssid, ssid, strlen(ssid));
  tmpParam.ssid[strlen(ssid)] = '\0';

  tmpParam.password = new char[strlen(password)+1];
  memcpy(tmpParam.password, password, strlen(password));
  tmpParam.password[strlen(password)] = '\0';

  return tmpParam;
}

netParam getNetConf()
{
  netParam tmpParam;
  tmpParam.localIP = WiFi.localIP();
  tmpParam.gatewayIP = WiFi.gatewayIP();
  tmpParam.dnsIP = WiFi.dnsIP();
  tmpParam.subnetMask = WiFi.subnetMask();
  return tmpParam;
}