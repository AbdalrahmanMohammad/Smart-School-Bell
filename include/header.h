#include <Arduino.h>
#include <LED.h>
#include <Bulb.h>
// #include "Timer.h"
#include <Wire.h>
#include "RTClib.h"
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>


// Create a web server on port 80
ESP8266WebServer server(80);


#define SCL D1
#define SDA D2
RTC_DS3231 rtc;


LED led(D7, D6);
Bulb bulb(D5, D3); // D3 has a button

// Global flag to stop LED operation
bool stopLedFlag = false;

#define DEBUG_SERIAL false

#if DEBUG_SERIAL
    #define dbg(...) Serial.print(__VA_ARGS__)
    #define dbgln(...) Serial.println(__VA_ARGS__)
    #define initDebugSerial() Serial.begin(9600)
#else
    #define dbg(...)
    #define dbgln(...)
    #define initDebugSerial()
#endif
