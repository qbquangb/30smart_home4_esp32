#include <Arduino.h>
#include "secrets.h"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

const int SENSOR_CUA = 3; // Sensor cua 1, phia duoi, co tac dong -> LOW, khong tac dong -> HIGH, dang su dung

const int LOA = 10; // Dieu khien loa
const int MAYTINH = 20; // Relay bat may tinh
const int BUZZER = 7; // Dieu khien BUZZER
const int DEN    = 4; // Dieu khien den

#define BIP_1      1
#define BIP_2      2
#define BIP_3      3
#define BIP_4      4

bool trangthaibaove = false;
bool isRunFirst = true;

void control_BUZZER(uint8_t para) {
    if (para == BIP_2) {
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(2000);
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(2000);
}
    if (para == BIP_1) {
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(2000);
    }
    if (para == BIP_3)
    {
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(2000);
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(2000);
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(2000);
    }
    if (para == BIP_4)
    {
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(2000);
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(2000);
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(2000);
        digitalWrite(BUZZER, HIGH);
        delay(500);
        digitalWrite(BUZZER, LOW);
        delay(2000);
    }
}

BlynkTimer timer;

void turnOffAction()
{
  digitalWrite(MAYTINH, LOW);
  Blynk.virtualWrite(V2, 0);   // cập nhật trạng thái MAYTINH trên app
  Blynk.virtualWrite(V3, 0);   // đưa nút về OFF
}

BLYNK_WRITE(V1)
{
  int pinValue = param.asInt();
  if (pinValue == 1)
  {
    digitalWrite(DEN, HIGH);
    Blynk.virtualWrite(V0, HIGH);
  }
  else
  {
    digitalWrite(DEN, LOW);
    Blynk.virtualWrite(V0,LOW);
  }
}

BLYNK_WRITE(V3)
{
  int pinValue = param.asInt();

  if (pinValue == 1)
  {
    digitalWrite(MAYTINH, HIGH);
    Blynk.virtualWrite(V2, 1);

    timer.setTimeout(500, turnOffAction);  // sau 500 ms tự tắt và đưa button về OFF
  }
}

BLYNK_WRITE(V5)
{
  int pinValue = param.asInt();
  if (pinValue == 1)
  {
    digitalWrite(LOA, HIGH);
    Blynk.virtualWrite(V4, HIGH);
  }
  else
  {
    digitalWrite(LOA, LOW);
    Blynk.virtualWrite(V4,LOW);
  }
}

BLYNK_WRITE(V7)
{
  int pinValue = param.asInt();
  if (pinValue == 1)
  {
    trangthaibaove = true;
    control_BUZZER(BIP_2);
  }
  else
  {
    trangthaibaove = false;
    control_BUZZER(BIP_4);
  }
}

void setup()
{
  pinMode(SENSOR_CUA, INPUT);
  pinMode(LOA, OUTPUT);
  pinMode(MAYTINH, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(DEN, OUTPUT);
  digitalWrite(LOA, LOW);
  digitalWrite(MAYTINH, LOW);
  digitalWrite(BUZZER, LOW);
  digitalWrite(DEN, LOW);

  trangthaibaove = false;
  isRunFirst = true;

  Blynk.begin(BLYNK_AUTH_TOKEN, WIFI_SSID, WIFI_PASSWORD);
  control_BUZZER(BIP_2);
  Blynk.virtualWrite(V0,LOW);
  Blynk.virtualWrite(V1,LOW);
  Blynk.virtualWrite(V2,LOW);
  Blynk.virtualWrite(V3,LOW);
  Blynk.virtualWrite(V4,LOW);
  Blynk.virtualWrite(V5,LOW);
  Blynk.virtualWrite(V6,LOW);
  Blynk.virtualWrite(V7,LOW);
}

void loop()
{
  Blynk.run();
  timer.run();

  if (digitalRead(SENSOR_CUA) == HIGH && trangthaibaove == true && isRunFirst == true) {
    delay(2000);
    if (digitalRead(SENSOR_CUA) == HIGH) {

    digitalWrite(BUZZER, HIGH);
    delay(20000);
    digitalWrite(BUZZER, LOW);

    // Cập nhật lên blynk
    Blynk.virtualWrite(V6, 1);

    // Thay đổi biến isRunFirst
    isRunFirst = false;

  }
}

}
