
// Global variables for schedule caching
bool schedulesCacheValid = false;
int cachedDayOfYear = -1; // Track which day is cached
String cachedOnTime = "";
String cachedOffTime = "";
int cachedOnMinutes = -1;  // Pre-calculated minutes for performance
int cachedOffMinutes = -1; // Pre-calculated minutes for performance

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
    pinMode(SCL, OUTPUT);       // SCL
    pinMode(SDA, INPUT_PULLUP); // SDA
    for (int i = 0; i < 16; i++)
    {
        if (digitalRead(SDA) == HIGH)
            break; // SDA released
        digitalWrite(SCL, LOW);
        delayMicroseconds(5);
        digitalWrite(SCL, HIGH);
        delayMicroseconds(5);
    }

    Wire.begin(SDA, SCL); // SDA = D2, SCL = D1

    if (!rtc.begin())
    {
        dbgln("Couldn't find RTC");
        while (1)
            ;
    }
    // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

// Function to load today's schedules into cache
void loadTodaysSchedulesToCache()
{
    dbgln("=== Loading today's schedules to cache ===");

    // Clear existing cache
    cachedOnTime = "";
    cachedOffTime = "";
    cachedDayOfYear = -1;
    cachedOnMinutes = -1;
    cachedOffMinutes = -1;

    // Get current day of year
    DateTime now = rtc.now();
    dbgln("Current RTC time: " + String(now.year()) + "-" + String(now.month()) + "-" + String(now.day()) + " " + String(now.hour()) + ":" + String(now.minute()));

    int currentDayOfYear = 0;
    int daysInMonth[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    for (int i = 0; i < now.month() - 1; i++)
    {
        currentDayOfYear += daysInMonth[i];
    }
    currentDayOfYear += now.day();

    dbgln("Calculated day of year: " + String(currentDayOfYear));

    // Read schedules from JSON file using streaming approach
    dbgln("Opening schedules.json for streaming...");
    File file = LittleFS.open("/schedules.json", "r");
    if (!file)
    {
        schedulesCacheValid = false;
        dbgln("ERROR: schedules.json not found");
        return;
    }

    dbgln("Found schedules.json, using streaming approach");

    bool foundToday = false;
    String buffer = "";
    int bracketCount = 0;
    bool inSchedulesArray = false;
    bool inScheduleObject = false;
    String currentSchedule = "";
    int processedSchedules = 0;

    dbgln("Streaming through JSON file...");

    // Read file character by character
    while (file.available() && !foundToday)
    {
        char c = file.read();
        buffer += c;

        // Keep buffer small (max 100 chars)
        if (buffer.length() > 100)
        {
            buffer = buffer.substring(buffer.length() - 50); // Keep last 50 chars
        }

        if (c == '[' && buffer.indexOf("\"schedules\"") != -1)
        {
            inSchedulesArray = true;
            dbgln("Found schedules array start");
            continue;
        }

        if (inSchedulesArray && c == '{')
        {
            bracketCount++;
            inScheduleObject = true;
            currentSchedule = "{";
            continue;
        }

        if (inScheduleObject)
        {
            currentSchedule += c;

            if (c == '{')
                bracketCount++;
            if (c == '}')
                bracketCount--;

            // Complete schedule object found
            if (bracketCount == 0 && c == '}')
            {
                processedSchedules++;

                // Parse this single schedule
                DynamicJsonDocument scheduleDoc(128); // Reduced size for better performance
                DeserializationError error = deserializeJson(scheduleDoc, currentSchedule);

                if (!error)
                {
                    int scheduleDay = scheduleDoc["d"].as<int>();

                    if (processedSchedules <= 5 || scheduleDay == currentDayOfYear)
                    {
                        dbgln("Schedule " + String(processedSchedules) + ": Day " + String(scheduleDay) + " vs target " + String(currentDayOfYear));
                    }

                    if (scheduleDay == currentDayOfYear)
                    {
                        // Found today's schedule!
                        cachedDayOfYear = currentDayOfYear;
                        cachedOnTime = scheduleDoc["on"].as<String>();
                        cachedOffTime = scheduleDoc["off"].as<String>();

                        // Performance optimization: Pre-calculate minutes here
                        const char *onTime = cachedOnTime.c_str();
                        const char *offTime = cachedOffTime.c_str();
                        cachedOnMinutes = ((onTime[0] - '0') * 10 + (onTime[1] - '0')) * 60 +
                                          ((onTime[3] - '0') * 10 + (onTime[4] - '0'));
                        cachedOffMinutes = ((offTime[0] - '0') * 10 + (offTime[1] - '0')) * 60 +
                                           ((offTime[3] - '0') * 10 + (offTime[4] - '0'));

                        foundToday = true;

                        dbgln("SUCCESS: Found today's schedule: " + cachedOnTime + " to " + cachedOffTime);
                        break;
                    }
                }

                inScheduleObject = false;
                currentSchedule = "";
            }
        }

        // Stop if we've processed enough schedules (optimization)
        if (processedSchedules > currentDayOfYear + 10)
        {
            dbgln("Stopping search after " + String(processedSchedules) + " schedules");
            break;
        }
    }

    file.close();

    dbgln("Processed " + String(processedSchedules) + " schedules via streaming");

    if (!foundToday)
    {
        dbgln("WARNING: No schedules found for today (day " + String(currentDayOfYear) + ")");
        dbgln("Using fallback schedule for today - ON at 18:00, OFF at 06:00");

        // Set fallback schedule for today
        cachedDayOfYear = currentDayOfYear;
        cachedOnTime = "18:00";
        cachedOffTime = "06:00";

        // Performance optimization: Pre-calculate fallback minutes
        cachedOnMinutes = 18 * 60; // 18:00 = 1080 minutes
        cachedOffMinutes = 6 * 60; // 06:00 = 360 minutes

        foundToday = true;
    }

    schedulesCacheValid = true;
    dbgln("Today's schedules loaded to cache successfully (streaming approach)");
}

// Function to initialize schedules cache (call in setup)
void initSchedulesCache()
{
    loadTodaysSchedulesToCache();
}

// Performance optimization: Cache day-of-year calculation
int cachedCurrentDayOfYear = -1;
int cachedCurrentYear = -1;
int cachedCurrentMonth = -1;
int cachedCurrentDay = -1;

void checkSchedules()
{
    // Performance optimization: Early exit if LED is OFF (bulb schedule disabled)
    if (!led.isOn())
    {
        return; // LED acts as master switch - if OFF, bulb schedule is disabled
    }

    // Get current time first
    DateTime now = rtc.now();
    int currentYear = now.year();
    int currentMonth = now.month();
    int currentDay = now.day();

    // Performance optimization: Only calculate day of year if date changed
    int currentDayOfYear;
    if (cachedCurrentYear != currentYear || cachedCurrentMonth != currentMonth || cachedCurrentDay != currentDay)
    {
        cachedCurrentYear = currentYear;
        cachedCurrentMonth = currentMonth;
        cachedCurrentDay = currentDay;

        currentDayOfYear = 0;
        const int daysInMonth[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

        for (int i = 0; i < currentMonth - 1; i++)
        {
            currentDayOfYear += daysInMonth[i];
        }
        currentDayOfYear += currentDay;
        cachedCurrentDayOfYear = currentDayOfYear;
    }
    else
    {
        currentDayOfYear = cachedCurrentDayOfYear;
    }

    if (currentYear < 2025 || currentYear > 2080)
    {
        dbgln("ERROR: Invalid year detected: " + String(currentYear) + ". Skipping schedule check.");
        return;
    }

    // Check if cache is valid and for today
    if (!schedulesCacheValid || cachedDayOfYear != currentDayOfYear || cachedOnMinutes == -1 || cachedOffMinutes == -1)
    {
        dbgln("Cache invalid or day changed, reloading...");
        loadTodaysSchedulesToCache();
        return; // No valid schedules to check
    }

    // Performance optimization: Use pre-calculated minutes from cache
    int currentMinutes = now.hour() * 60 + now.minute();

    // Determine which time comes first (first) and which comes second (second)
    int firstTime, secondTime;

    if (cachedOnMinutes < cachedOffMinutes)
    {
        // ON comes first, OFF comes second
        firstTime = cachedOnMinutes;
        secondTime = cachedOffMinutes;
    }
    else
    {
        // OFF comes first, ON comes second
        firstTime = cachedOffMinutes;
        secondTime = cachedOnMinutes;
    }

    // Apply time-range logic with performance optimization
    if (currentMinutes < firstTime && currentMinutes < secondTime)
    {
        // Current time is before both times - apply the state of the second time
        if (cachedOnMinutes < cachedOffMinutes)
        {
            // ON comes first, OFF comes second - so before both means OFF state
            bulb.off();
        }
        else
        {
            // OFF comes first, ON comes second - so before both means ON state
            bulb.on();
        }
    }
    else if (currentMinutes > firstTime && currentMinutes >= secondTime)
    {
        // Current time is after both times - apply the state of the second time
        if (cachedOnMinutes < cachedOffMinutes)
        {
            // ON comes first, OFF comes second - so after both means OFF state
            bulb.off();
        }
        else
        {
            // OFF comes first, ON comes second - so after both means ON state
            bulb.on();
        }
    }
    else if (currentMinutes >= firstTime && currentMinutes < secondTime)
    {
        // Current time is between the two times
        if (cachedOnMinutes < cachedOffMinutes)
        {
            // ON comes first, so we're in the ON period
            bulb.on();
        }
        else
        {
            // OFF comes first, so we're in the OFF period
            bulb.off();
        }
    }
}

// Performance optimization: Add timing control for schedule checking
unsigned long lastScheduleCheck = 0;
const unsigned long SCHEDULE_CHECK_INTERVAL = 10000; // Check every 10 seconds instead of every loop

void controlDevices()
{
    led.loop();
    if (!led.isOn())
        bulb.loop();

    if (led.gotOff)
    {
        bulb.persistState();
        led.gotOff = false;
    }

    if (led.checkNow) // Check if LED state just changed to ON
    {
        checkSchedules(); // Immediate sync
        led.checkNow = false;
    }

    unsigned long currentTime = millis();
    if (currentTime - lastScheduleCheck >= SCHEDULE_CHECK_INTERVAL)
    {
        checkSchedules(); // Regular schedule check
        lastScheduleCheck = currentTime;
    }
}

void applySavedConfig()
{
    StaticJsonDocument<128> cfg; // Reduced size for better performance
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

    // Bulb last state (only apply if LED is OFF - manual control mode)
    if (!ledOn)
    {
        bool bulbOn = cfg.containsKey("bulbOn") ? cfg["bulbOn"].as<bool>() : false;
        if (bulbOn)
        {
            bulb.on();
        }
        else
        {
            bulb.off();
        }
        dbgln("Bulb state restored: " + String(bulbOn ? "ON" : "OFF"));
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
