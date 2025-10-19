#include <header.h>
#include <func.h>

void setup()
{
  delay(500); // wait for a second
  initDebugSerial();
  initLittleFS();
  led.init();
  bulb.init();
  applySavedConfig();
  delay(500);
  WifiSetup();
  RtcSetup();
  initSchedulesCache(); // Load schedules once at startup
  delay(500);
}

void loop()
{
  server.handleClient(); // handle incoming client requests
  // showTime();
  controlDevices();
}
