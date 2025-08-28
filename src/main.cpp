#include <header.h>
#include <func.h>

void setup()
{
  Serial.begin(9600);
  led.init();
  bell.init();

  Wire.begin(SDA, SCL); // SDA = D2, SCL = D1

  if (!rtc.begin())
  {
    Serial.println("Couldn't find RTC");
    while (1)
      ;
  }
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void loop()
{

  DateTime now = rtc.now();

  //   if (rtc.lostPower()) {
  //   Serial.println("RTC lost power detected!");
  //   // You could reset or ask user to re-set time
  // }

  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();

  // checkWifi();

  // float x = measure(30);

  control();
}
