#include <header.h>
#include <func.h>

void setup()
{
  Serial.begin(9600);
  led.init();
  // pinMode(buzzer, OUTPUT);
  // pinMode(wifiLed, OUTPUT);
  // connect_wifi();
}

void loop()
{
  // checkWifi();

  // float x = measure(30);

  control();
}
