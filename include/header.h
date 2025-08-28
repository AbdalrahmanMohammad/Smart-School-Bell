#include <Arduino.h>
#include <LED.h>
#include <Bell.h>
#include "Timer.h"
#include <Wire.h>
#include "RTClib.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>


// Create a web server on port 80
ESP8266WebServer server(80);


#define SCL D1
#define SDA D2
RTC_DS3231 rtc;


Timer timer(20UL);
LED led(D7, D6);
Bell bell(D5);
