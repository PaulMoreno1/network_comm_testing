#include <Arduino.h>
#include <WiFi.h>

#define NETOCTET_1     65
#define NETOCTET_2     73
#define NETOCTET_3      0
#define NETOCTET_4      1

#define FULLOCTET   255
#define NULLOCTET     0

IPAddress AP_IP;
IPAddress AP_gateway;
IPAddress AP_subnet;
IPAddress IP_dhcp;

void setAP_params(
            IPAddress AP_LocalIP    = IPAddress(NETOCTET_1, NETOCTET_2, NETOCTET_3, NETOCTET_4+1),
            IPAddress AP_GatewayIP  = IPAddress(NETOCTET_1, NETOCTET_2, NETOCTET_3, NETOCTET_4),
            IPAddress AP_SubnetMask = IPAddress(FULLOCTET, FULLOCTET, FULLOCTET, NULLOCTET),
            IPAddress AP_DCHPf      = IPAddress(NETOCTET_1, NETOCTET_2, NETOCTET_3, NETOCTET_4+100))
{
  AP_IP = AP_LocalIP;
  AP_gateway = AP_GatewayIP;
  AP_subnet = AP_SubnetMask;
  IP_dhcp = AP_DCHPf; 
}

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_MODE_AP);
  const char* AP_SSID = "AIOS Access Point";
  const char* AP_password = "AIOS2024";
  WiFi.softAP(AP_SSID, AP_password);
  //setAP_params();
  IPAddress lK_AP_IP(168,192,0,100);
  IPAddress lK_AP_gateway(168,192,0,1);
  IPAddress lK_AP_subnet(255,255,255,0);
  IPAddress lK_IP_dhcp(168,192,0,2);

  setAP_params(lK_AP_IP,lK_AP_gateway,lK_AP_subnet,lK_IP_dhcp);
  //setAP_params();
  Serial.print("(IP)\n\tReg: ");
  Serial.println(AP_IP);
  Serial.print("\tExp: ");
  Serial.println(lK_AP_IP);
  Serial.print("(Gateway)\n\tReg: ");
  Serial.println(AP_gateway);
  Serial.print("\tExp: ");
  Serial.println(lK_AP_gateway);
  Serial.print("(Subnet)\n\tReg: ");
  Serial.println(AP_subnet);
  Serial.print("\tExp: ");
  Serial.println(lK_AP_subnet);
  Serial.print("(DHCP)\n\tReg: ");
  Serial.println(IP_dhcp);
  Serial.print("\tExp: ");
  Serial.println(lK_IP_dhcp);
  WiFi.softAPConfig(AP_IP, AP_gateway, AP_subnet, IP_dhcp);
  Serial.print(WiFi.softAPIP());
}

void loop()
{

}