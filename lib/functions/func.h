#include <webPage.h>

void initLittleFS()
{
    if (!LittleFS.begin())
    {
        Serial.println("LittleFS initialization failed!");
        return;
    }
    Serial.println("LittleFS initialized successfully!");
    Serial.println("Files on LittleFS:");
    Dir dir = LittleFS.openDir("/"); // Root directory
    while (dir.next())
    {
        Serial.print(dir.fileName());
        Serial.print("  \t");
        Serial.println(dir.fileSize());
    }
}

void RtcSetup()
{
    Wire.begin(SDA, SCL); // SDA = D2, SCL = D1

    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");
        while (1)
            ;
    }
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void controlDevices()
{
    led.loop();
    bell.loop();
}

void showTime()
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
}