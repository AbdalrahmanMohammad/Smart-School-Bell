// Global variable to track last triggered time to prevent multiple triggers
String lastTriggeredTime = "";

// Global variables for schedule caching
DynamicJsonDocument *cachedSchedulesDoc = nullptr;
bool schedulesCacheValid = false;

void initLittleFS()
{
    if (!LittleFS.begin())
    {
        dbgln("LittleFS initialization failed!");
        return;
    }
    dbgln("LittleFS initialized successfully!");
    dbgln("Files on LittleFS:");
    Dir dir = LittleFS.openDir("/"); // Root directory
    while (dir.next())
    {
        dbg(dir.fileName());
        dbg("  \t");
        dbgln(dir.fileSize());
    }
}

void RtcSetup()
{
    Wire.begin(SDA, SCL); // SDA = D2, SCL = D1

    if (!rtc.begin())
    {
        dbgln("Couldn't find RTC");
        while (1)
            ;
    }
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

// Function to load schedules into cache
void loadSchedulesToCache()
{
    // Free existing cache if it exists
    if (cachedSchedulesDoc != nullptr)
    {
        delete cachedSchedulesDoc;
        cachedSchedulesDoc = nullptr;
    }

    // Read schedules from flash memory
    File file = LittleFS.open("/schedules.json", "r");
    if (!file)
    {
        schedulesCacheValid = false;
        dbgln("No schedules file found");
        return; // No schedules file
    }

    String jsonData = file.readString();
    file.close();

    // Allocate new document on heap
    cachedSchedulesDoc = new DynamicJsonDocument(16384);
    DeserializationError error = deserializeJson(*cachedSchedulesDoc, jsonData);

    if (error)
    {
        dbgln("Failed to parse schedules.json");
        delete cachedSchedulesDoc;
        cachedSchedulesDoc = nullptr;
        schedulesCacheValid = false;
        return;
    }

    schedulesCacheValid = true;
    dbgln("Schedules loaded to cache successfully");
}

// Function to initialize schedules cache (call in setup)
void initSchedulesCache()
{
    loadSchedulesToCache();
}

void checkSchedules()
{
    if (!led.isOn())
    {
        return;
    }

    // Check if cache is valid
    if (!schedulesCacheValid || cachedSchedulesDoc == nullptr)
    {
        dbgln("Schedules cache invalid, reloading...");
        loadSchedulesToCache();
        return; // No valid schedules to check
    }

    // Get current time
    DateTime now = rtc.now();
    int currentYear = now.year();
    dbgln("Current year: " + String(currentYear));
    if (currentYear < 2025 || currentYear > 2060)
    {
        dbgln("Invalid year detected: " + String(currentYear) + ". Skipping schedule check.");
        return;
    }
    int currentDayOfWeek = now.dayOfTheWeek(); // 0 = Sunday, 1 = Monday, etc.
    currentDayOfWeek++;
    if (currentDayOfWeek == 7)
        currentDayOfWeek = 0;
    String currentTime = "";
    if (now.hour() < 10)
        currentTime += "0";
    currentTime += String(now.hour());
    currentTime += ":";
    if (now.minute() < 10)
        currentTime += "0";
    currentTime += String(now.minute());

    dbgln("Current time: " + currentTime + " Day of week: " + String(currentDayOfWeek));
    // Prevent multiple triggers in the same minute
    if (currentTime == lastTriggeredTime)
    {
        return;
    }

    // Check each schedule using cached document
    JsonArray schedules = (*cachedSchedulesDoc)["schedules"];
    for (JsonObject schedule : schedules)
    {
        // Check if schedule is enabled
        if (!schedule["enabled"].as<bool>())
        {
            continue;
        }

        // Check if current day is in the schedule
        JsonArray days = schedule["days"];
        bool dayMatches = false;
        for (int day : days)
        {
            if (day == currentDayOfWeek)
            {
                dayMatches = true;
                break;
            }
        }

        if (!dayMatches)
        {
            continue;
        }

        // Check if current time matches schedule time
        const char *scheduleTime = schedule["time"];
        if (strcmp(scheduleTime, currentTime.c_str()) == 0)
        {
            // Time matches! Ring the bell
            const char *type = schedule["type"];
            if (strcmp(type, "bell") == 0)
            {
                dbg("Ringing bell at scheduled time: ");
                dbgln(scheduleTime);
                bell.on();
                lastTriggeredTime = currentTime; // Mark this time as triggered
            }
            else if (strcmp(type, "led") == 0)
            {
                dbg("Toggling LED at scheduled time: ");
                dbgln(scheduleTime);
                led.off();
                lastTriggeredTime = currentTime; // Mark this time as triggered
            }
        }
    }
}

void controlDevices()
{
    led.loop();
    bell.loop();
    checkSchedules(); // Add schedule checking
}

void applySavedConfig()
{
    StaticJsonDocument<256> cfg;
    File f = LittleFS.open("/config.json", "r");
    if (f)
    {
        String c = f.readString();
        f.close();
        DeserializationError e = deserializeJson(cfg, c);
        if (e)
        {
            cfg.clear();
        }
    }

    // Bell duration
    unsigned long bellDurationMs = cfg.containsKey("bellDurationMs") ? cfg["bellDurationMs"].as<unsigned long>() : 3000UL;
    bell.setDuration(bellDurationMs);

    // LED last state
    bool ledOn = cfg.containsKey("ledOn") ? cfg["ledOn"].as<bool>() : false;
    if (ledOn)
    {
        led.on();
    }
    else
    {
        led.off();
    }
}

void showTime()
{
    DateTime now = rtc.now();

    //   if (rtc.lostPower()) {
    //   dbgln("RTC lost power detected!");
    //   // You could reset or ask user to re-set time
    // }

    dbg(now.year(), DEC);
    dbg('/');
    dbg(now.month(), DEC);
    dbg('/');
    dbg(now.day(), DEC);
    dbg(" ");
    dbg(now.hour(), DEC);
    dbg(':');
    dbg(now.minute(), DEC);
    dbg(':');
    dbg(now.second(), DEC);
    dbgln();
}
#include <webPage.h>
