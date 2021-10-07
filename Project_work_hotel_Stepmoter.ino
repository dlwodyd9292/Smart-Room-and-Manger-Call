/*
 WiFiEsp test: ClientTest
http://www.kccistc.net/
작성일 : 2019.12.17 
작성자 : IoT 임베디드 KSH
*/
//#define DEBUG
//#define DEBUG_WIFI

#define AP_SSID "iotsystem0"
#define AP_PASS "iotsystem00"
#define SERVER_NAME "192.168.10.74"
#define SERVER_PORT 5000  
#define LOGID "KSP_ARD2"
#define PASSWD "PASSWD"

#define WIFITX 7  //7:TX -->ESP8266 RX
#define WIFIRX 6 //6:RX-->ESP8266 TX
#define BlindUPLED 4
#define BlindDOWNLED 3
 
#define STEPS 8192
#define RPM 15
#define IN1 8 // IN1
#define IN2 9 // IN2
#define IN3 10 // IN3
#define IN4 11 // IN4


#define CMD_SIZE 50
#define ARR_CNT 5           

#include "WiFiEsp.h"
#include "SoftwareSerial.h"
#include <Stepper.h>

Stepper mystepper(2048, IN4, IN2, IN3, IN1);

char sendBuf[CMD_SIZE];
char sendID[10]="KSP_LDP";

SoftwareSerial wifiSerial(WIFIRX, WIFITX); 
WiFiEspClient client;

void setup() 
{
    pinMode (BlindUPLED, OUTPUT);   
    pinMode (BlindDOWNLED, OUTPUT); 
    mystepper.setSpeed(RPM);
   
    Serial.begin(115200); //DEBUG
    wifi_Setup();
}

void loop() 
{
  if(client.available()) 
  {
    socketEvent();
  }
}
void socketEvent()
{
  int i=0;
  char * pToken;
  char * pArray[ARR_CNT]={0};
  char recvBuf[CMD_SIZE]={0}; 
  int len;

  sendBuf[0] ='\0';
  len =client.readBytesUntil('\n',recvBuf,CMD_SIZE); 
  client.flush();
#ifdef DEBUG
  Serial.print("recv : ");
  Serial.print(recvBuf);
#endif
  pToken = strtok(recvBuf,"[@]");
  while(pToken != NULL)
  {
    pArray[i] =  pToken;
    if(++i >= ARR_CNT)
      break;
    pToken = strtok(NULL,"[@]");
  }
  if(!strncmp(pArray[1]," Alr",4)) //Already logged
  {
    Serial.write('\n');
    client.stop();
    server_Connect();
    return ;
  }   
  else if(!strcmp(pArray[1],"Blind")) 
  {
    if(!strcmp(pArray[2],"UP")) 
    {
      digitalWrite(BlindUPLED,HIGH);
      digitalWrite(BlindDOWNLED,LOW);
      mystepper.step(STEPS);
    }
    else if(!strcmp(pArray[2],"DOWN")) 
    {
      digitalWrite(BlindUPLED,LOW);
      digitalWrite(BlindDOWNLED,HIGH);
      mystepper.step(-STEPS);
    }
    digitalWrite(BlindUPLED,LOW);
    digitalWrite(BlindDOWNLED,LOW);
    sprintf(sendBuf,"[%s]%s@%s\n",pArray[0],pArray[1],pArray[2]);
  } 
  else if(!strcmp(pArray[1],"GETSTATE")) 
  {
    strcpy(sendID, pArray[0]);
    if(!strcmp(pArray[2],"DEV")) 
    {
      sprintf(sendBuf,"[%s]DEV@%s\n",pArray[0]);
    }
  }
  
  client.write(sendBuf,strlen(sendBuf));
  client.flush();

#ifdef DEBUG
  Serial.print(", send : ");
  Serial.print(sendBuf);
#endif
}
void wifi_Setup() {
  wifiSerial.begin(19200);
  wifi_Init();
  server_Connect();
}
void wifi_Init()
{
  do {
    WiFi.init(&wifiSerial);
    if (WiFi.status() == WL_NO_SHIELD) {
#ifdef DEBUG_WIFI    
      Serial.println("WiFi shield not present");
#endif 
    }
    else
      break;   
  }while(1);

#ifdef DEBUG_WIFI    
  Serial.print("Attempting to connect to WPA SSID: ");
  Serial.println(AP_SSID);
#endif     
  while(WiFi.begin(AP_SSID, AP_PASS) != WL_CONNECTED) {   
#ifdef DEBUG_WIFI  
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(AP_SSID);   
#endif   
  }
#ifdef DEBUG_WIFI      
  Serial.println("You're connected to the network");    
  printWifiStatus();
#endif 
}
int server_Connect()
{
#ifdef DEBUG_WIFI     
  Serial.println("Starting connection to server...");
#endif  

  if (client.connect(SERVER_NAME, SERVER_PORT)) {
#ifdef DEBUG_WIFI     
    Serial.println("Connected to server");
#endif  
    client.print("["LOGID":"PASSWD"]"); 
  }
  else
  {
#ifdef DEBUG_WIFI      
     Serial.println("server connection failure");
#endif    
  } 
}
void printWifiStatus()
{
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
