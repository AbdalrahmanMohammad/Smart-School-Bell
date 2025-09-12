#define BLYNK_TEMPLATE_ID "TMPL6M8XdEaaq"
#define BLYNK_TEMPLATE_NAME "Smart bell"
#define BLYNK_AUTH_TOKEN "93DqM2upTspUWRxU235gv5q7xP9v7Sly"

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
  // Maintain WiFi/Blynk without blocking other tasks
  maintainConnectivity();
  if (Blynk.connected())
  {
    Blynk.run();           // cloud control
  }
  server.handleClient(); // local web control
  // showTime();
  controlDevices();
}
