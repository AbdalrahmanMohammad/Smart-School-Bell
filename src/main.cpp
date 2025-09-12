#include <header.h>
#include <func.h>

void setup()
{
  initDebugSerial();
  initLittleFS();
  led.init();
  bell.init();
  applySavedConfig();
  WifiSetup();
  RtcSetup();
  initSchedulesCache(); // Load schedules once at startup
}

void loop()
{
  server.handleClient(); // handle incoming client requests
  // showTime();
  controlDevices();
}
