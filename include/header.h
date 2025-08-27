#include <Arduino.h>
#include <LED.h>
#include "Timer.h"

#define SDA D1
#define SCL D2

Timer timer(20UL);
LED led(D8, D6);

int buzzer = D8;
