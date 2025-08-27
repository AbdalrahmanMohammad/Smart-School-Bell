#include <Arduino.h>
#include <LED.h>
#include <Bell.h>
#include "Timer.h"

#define SDA D1
#define SCL D2

Timer timer(20UL);
LED led(D8, D6);
Bell bell(D3, D1);

int buzzer = D8;
