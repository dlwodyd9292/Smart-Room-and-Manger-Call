#define DEBUG
//#define DEBUG_WIFI

#define AP_SSID "iotsystem0"
#define AP_PASS "iotsystem00"
#define SERVER_NAME "192.168.10.74"
#define SERVER_PORT 5000  
#define LOGID "LJY_ARD1"
#define PASSWD "PASSWD"

#define WIFITX 7  //7:TX -->ESP8266 RX
#define WIFIRX 6 //6:RX-->ESP8266 TX

#define LED_LAMP_PIN 3
#define SERVO_PIN 13

#define CMD_SIZE 50
#define ARR_CNT 5    

#include "WiFiEsp.h"
#include "SoftwareSerial.h"

#include <MsTimer2.h>
#include <Wire.h>
#include <Servo.h>

char sendBuf[CMD_SIZE];

bool timerIsrFlag = false;
unsigned int secCount;

char sendId[10]="LJY_LDP";

SoftwareSerial wifiSerial(WIFIRX, WIFITX); 
WiFiEspClient client;

Servo myservo;
bool myservoFlag=false;

char lcdLine1[17]="Smart IoT By KSH";
char lcdLine2[17]="WiFi Connecting!";
boolean lastButton = LOW;     // 버튼의 이전 상태 저장

void setup() {
  // put your setup code here, to run once:
    myservo.attach(SERVO_PIN);  
    myservo.write(0);
  
    
    pinMode (LED_LAMP_PIN, OUTPUT);    // LED 핀을 출력으로 설정
 
    Serial.begin(115200); //DEBUG

    wifi_Setup();

    MsTimer2::set(1000, timerIsr); // 1000ms period
    MsTimer2::start();  
}

void loop() {
  // put your main code here, to run repeatedly:
  if(client.available()) {
    socketEvent();
  }
  if (timerIsrFlag)
  {
    timerIsrFlag = false; 
    if(!(secCount%5))
    {
      if (!client.connected()) { 
  
        server_Connect();
      }
    } 
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
  //[KSH_ARD]LED@ON : pArray[0] = "KSH_ARD", pArray[1] = "LED", pArray[2] = "ON"
  if((strlen(pArray[1]) + strlen(pArray[2])) < 16)
  {   
  }
  if(!strncmp(pArray[1]," New",4))  // New Connected
  {
    Serial.write('\n');
    return ;
  }
  else if(!strncmp(pArray[1]," Alr",4)) //Already logged
  {
    Serial.write('\n');
    client.stop();
    server_Connect();
    return ;
  }   
  
   else if(!strcmp(pArray[1],"SERVO")) 
  {
    myservoFlag = true;
    if(!strcmp(pArray[2],"ON"))
    {
      myservo.write(0); 
      digitalWrite(LED_LAMP_PIN, HIGH);
    }
    else if(!strcmp(pArray[2],"CLOSE"))
    {
      myservo.write(110);
      digitalWrite(LED_LAMP_PIN, LOW);
    }
    sprintf(sendBuf,"[%s]%s@%s\n",pArray[0],pArray[1],pArray[2]);
  } 
  else
      return;
  
  client.write(sendBuf,strlen(sendBuf));
  client.flush();

#ifdef DEBUG
  Serial.print(", send : ");
  Serial.print(sendBuf);
#endif
}
void timerIsr()
{
//  digitalWrite(LED_BUILTIN_PIN,!digitalRead(LED_BUILTIN_PIN));
  timerIsrFlag = true;
  secCount++;
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
    Serial.println("Connect to server");
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
