#include <Arduino.h>
#include <LED.h>
#include <Bell.h>
#include "Timer.h"
#include <Wire.h>
#include "RTClib.h"

#define SCL D1
#define SDA D2
RTC_DS3231 rtc;


Timer timer(20UL);
LED led(D7, D6);
Bell bell(D5);
