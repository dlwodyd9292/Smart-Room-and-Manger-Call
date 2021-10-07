/*
 WiFiEsp test: ClientTest
http://www.kccistc.net/
작성일 : 2021.08.04 
작성자 : IoT 임베디드 LJY
*/
#define DEBUG
//#define DEBUG_WIFI //와이파이 정보 확인하고 싶으면 주석 해제, 디버그용

#define AP_SSID "iotsystem2"
#define AP_PASS "iotsystem20"
#define SERVER_NAME "192.168.10.74"
#define SERVER_PORT 5000  
#define LOGID "LJY_ARD"
#define PASSWD "PASSWD"

#define WIFITX 7  //7:TX -->ESP8266 RX
#define WIFIRX 6 //6:RX-->ESP8266 TX

#define DOOR_PIN 3

#define CMD_SIZE 50 //for recv & send buf size
#define ARR_CNT 5 //문자열 파싱할 때 파싱한 개수를 구분하기 위한 카운터 디파인
  
#define DHTPIN 10     // Digital pin connected to the DHT sensor 
#define DHTTYPE DHT11   // DHT 11       

#include "WiFiEsp.h"
#include "SoftwareSerial.h"
#include <TimerOne.h> //아두이노 MCU인 AVR328의 0, 1, 2 타이머 중 1번 타이머를 쓰겠다
#include <Wire.h> //I2C
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

LiquidCrystal_I2C lcd(0x27, 16, 2); //I2C LCD init

char sendBuf[CMD_SIZE];

bool timerIsrFlag = false; //타이머 발생여부 체크 플래그
unsigned int secCount; //최대 65535까지 카운트 가능 

char sendId[10]= "LJY_LDP";
//char* sendId = "LJY_SMP"; //반환받은 아이디 초기화 
//새로운 값으로 복사는 안된다. 동적 메모리를 할당해야한다. 
//k의 주소값을 저장하기 때문에 readonly 영역을 발생하기 때문에 멈춰야 정상

SoftwareSerial wifiSerial(WIFIRX, WIFITX); //seiral init
WiFiEspClient client; //for socket client

DHT dht(DHTPIN, DHTTYPE);

char lcdLine1[17]="Smart IoT By LJY"; //null문자까지 포함해서 16+1 = 17
char lcdLine2[17]="WiFi Connecting!"; 
boolean lastButton = LOW;     // 버튼의 이전 상태 저장
boolean currentButton = LOW;    // 버튼의 현재 상태 저장
boolean ledOn = false;      // LED의 현재 상태 (on/off)
boolean cdsFlag = false; //50% 이상 이하 구분을 위한 불리언(참,거짓)플래그 

int h; //Humidity
int t; //Temperature

void setup() {
    
  // put your setup code here, to run once:
    lcd.init(); //클래스 = 멤버변수 + 멤버함수, C++
    lcd.backlight();
    lcdDisplay(0,0,lcdLine1);//x축 가로, y축 세로
    lcdDisplay(0,1,lcdLine2);
     
    Serial.begin(115200); //DEBUG
    wifi_Setup(); //처음 AP에 연결되었을 때만 실행

    dht.begin();

    Timer1.initialize(1000000); 
     //마이크로초 단위, 100만 마이크로초 = 1초
     //1000 = 1밀리초 
    Timer1.attachInterrupt(timerIsr); // timerIsr 호출, timerIsr to run every 1 seconds
}

void loop() {
  // put your main code here, to run repeatedly:
  if(client.available()) { //클라이언트에서 데이터가 들어왔으면
    socketEvent();
  } 
//reuse possible
  if (timerIsrFlag) //1초마다 셋
  {
    timerIsrFlag = false; //무한정 루프를 도는 것을 방지하기 위한 셋 번경 
    if(!(secCount%5)) //5초마다 자동으로 서버 접속 시도하는 부분 
    {
      if (!client.connected()) { //클라이언트 연결 종료시 
        lcdDisplay(0,1,"Server Down");
        server_Connect(); //계속 서버 커넥트 시도 
      } 
    }
     
    if(!(secCount%5))
    { 
      if (isnan(h) || isnan(t)) 
      {
        Serial.println("Failed to read from DHT sensor!");
      }

      h = dht.readHumidity();
      t = dht.readTemperature();

      sprintf(lcdLine1,"Humi : %d%%", h);  
      lcdDisplay(0,0,lcdLine1);
      Serial.println(lcdLine1);
      sprintf(lcdLine2,"Temp : %dC", t);
      lcdDisplay(0,1,lcdLine2);
      Serial.println(lcdLine2);

      if(t >= 25) 
      {
        client.write("[KSP_ARD1]AC POWER@HIGH");
      } 
      else if((t >= 24) && (t < 25)) 
      {
        client.write("[KSP_ARD1]AC POWER@LOW");
      } 
      else if(t < 24) 
      {
        client.write("[KSP_ARD1]AC POWER@STOP");
      } 
     
    }     
  }
}
void socketEvent()
{
  int i=0;
  char * pToken;
  char * pArray[ARR_CNT]={0};
  char recvBuf[CMD_SIZE]={0}; //수신 데이터를 저장한 버퍼
  int len;

  sendBuf[0] ='\0'; 
  len =client.readBytesUntil('\n',recvBuf,CMD_SIZE);
  //client 버퍼에서 '\n' 이전까지 읽어와서 recvbuf에 저장 
  client.flush(); //'\n' 뒤의 데이터는 다 지운다 
#ifdef DEBUG
  Serial.print("recv : ");
  Serial.print(recvBuf);
#endif
  pToken = strtok(recvBuf,"[@]"); //해당 문자 기준으로 분리
  while(pToken != NULL) 
  {
    pArray[i] =  pToken; 
     if(++i >= ARR_CNT) //들어온 문자열을 토큰으로 분리했는데 그 수가 5개보다 많으면
      break; //탈출 
    pToken = strtok(NULL,"[@]"); 
  }

  //ex)[LJY_ARD]LED@ON 
  // = pArray[0] = "LJY_ARD", pArray[1] = "LED", pArray[2] = "ON" 
  
  if((strlen(pArray[1]) + strlen(pArray[2])) < 16) //받은 문자열 길이가 16개 미만이면
  {
    sprintf(lcdLine2,"%s %s",pArray[1],pArray[2]);
    lcdDisplay(0,1,lcdLine2);
  }
  if(!strncmp(pArray[1]," New",4))  // New Connected 앞에 한칸 띄워져 있으니 주의
  {
    Serial.write('\n');
    strcpy(lcdLine2,"Server Connected");
    lcdDisplay(0,1,lcdLine2);

    return ; //리턴 시 해당 처리 과정이 끝나고 밑의 코드는 실행이 안됨 
  }
  else if(!strncmp(pArray[1]," Alr",4)) //Already logged 앞에 한칸 띄워져 있으니 주의
  {
    Serial.write('\n');
    client.stop();
    server_Connect(); //와이파이가 아닌 우분투 서버와 연결
    return ; 
  }   
   else if(!strcmp(pArray[1],"GETSTATE")) {
    strcpy(sendId, pArray[0]); //GETSTATE 명령을 지시한 아이디를 sendId에 저장 
    if(!strcmp(pArray[2],"DEV")) {
      sprintf(sendBuf,"[%s]DEV@%s\n",pArray[0],digitalRead(DHTPIN)?"ON":"OFF");
    }
//<<**********************************************************************    
  } else
      return; 
      //위에 해당되는 문자열이 아니면 여기서 코드를 종료 
      //loop back 문제 해결을 위한 return(반환 안되므로 이 아래 코드는 실행 안됨)
//<<********************************************************************** 
  client.write(sendBuf,strlen(sendBuf));
  client.flush();

#ifdef DEBUG
  Serial.print(", send : ");
  Serial.print(sendBuf);
#endif
}
void timerIsr() //셋업에서 설정된 주기(1초)마다 호출될 함수 
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
  sprintf(lcdLine1,"ID:%s",LOGID);  
  lcdDisplay(0,0,lcdLine1);
  sprintf(lcdLine2,"%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  lcdDisplay(0,1,lcdLine2);
  
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
    // "[""LJY_ARD"":""PASSWD""]"
    // = "[LJY_ARD:PASSWD]"
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
void lcdDisplay(int x, int y, char * str)
{
  int len = 16 - strlen(str);
  lcd.setCursor(x,y);
  lcd.print(str);
  for(int i=len;i>0;i--)
    lcd.write(' ');
}
