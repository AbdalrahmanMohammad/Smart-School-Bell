#include <header.h>
#include <func.h>

void setup()
{
  delay(2000); // wait for a second
  initDebugSerial();
  initLittleFS();
  led.init();
  bulb.init();
  applySavedConfig();
  delay(1000);
  WifiSetup();
  RtcSetup();
  initSchedulesCache(); // Load schedules once at startup
  delay(1000);
}

void loop()
{
  server.handleClient(); // handle incoming client requests
  // showTime();
  controlDevices();
}
