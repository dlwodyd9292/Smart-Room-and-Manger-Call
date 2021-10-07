#include <Servo.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

Servo myServo;
LiquidCrystal_I2C lcd(0x27, 16, 2); 
 
#define trigPin 2 // Trigger Pin
#define echoPin 3 // Echo Pin

 
#define maximumRange   500  // 최대 거리
#define minimumRange   2 // 최소 거리

long duration, distance; // 펄스 시간, 거리 측정용 변수

int flag = 0;
 
// 프로그램 시작시 초기화 작업
void setup() 
{ 
  
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  myServo.attach(9);
  myServo.write(0);
  pinMode(7,INPUT);

  lcd.init();
  lcd.backlight();
}
 
// 계속 실행할 무한 루프
void loop() 
{
 digitalWrite(trigPin, LOW); 
 delayMicroseconds(2); 
 
 digitalWrite(trigPin, HIGH);
 delayMicroseconds(10); 
 
 digitalWrite(trigPin, LOW);
 duration = pulseIn(echoPin, HIGH);
 
 // 측정된 시간을 cm로 환산
 distance = duration/58.2;
 
 if(distance < 12) 
  { 
    myServo.write(90);
    lcd.setCursor(0,0);          
    lcd.print("Welcome !!"); 
    lcd.setCursor(0,1);          
    lcd.print("Manager !!");    

   flag = 1;
    
  }
  else if((distance >= 12) && (distance < 50) && (flag == 1))
  {
    myServo.write(0);
    lcd.setCursor(0,0);
    lcd.print("Have a Good day"); 
    lcd.setCursor(0,1);          
    lcd.print("Manager");  

    flag = 0;
  }
 else
 {
    lcd.setCursor(0,0);          
    lcd.print(" "); 
    lcd.setCursor(0,1);          
    lcd.print(" ");    
 }
 delay(1000);
 lcd.clear();
 }
