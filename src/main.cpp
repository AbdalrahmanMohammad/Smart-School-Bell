#include <header.h>
#include <func.h>

void setup()
{
  Serial.begin(9600);
  led.init();
  bell.init();
  WifiSetup();
  RtcSetup();
}

void loop()
{

  server.handleClient(); // handle incoming client requests
  showTime();
  controlDevices();
}
