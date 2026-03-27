#pragma once
#include <Arduino.h>

unsigned long timeoutDuration(unsigned long startTime);
char readCharFromSim(unsigned long timeout = 10000);
String readLineFromSim(unsigned long timeout = 10000);
bool sendSMS(String message);
bool readUntilResponseOrTimeout(const String &expectedResponse, unsigned long timeout, bool responsesProcess = false);
bool offBuzzerAfterTimeout(unsigned long startBuzzerOn, unsigned long duration = 60000);
void process(String &s);
void control_BUZZER(uint8_t para);
void turnOffAction();