
// Global variables for schedule caching
DynamicJsonDocument *cachedSchedulesDoc = nullptr;
bool schedulesCacheValid = false;
int cachedDayOfYear = -1; // Track which day is cached

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
    
    // Free existing cache if it exists
    if (cachedSchedulesDoc != nullptr)
    {
        delete cachedSchedulesDoc;
        cachedSchedulesDoc = nullptr;
        dbgln("Freed existing cache");
    }

    // Get current day of year
    DateTime now = rtc.now();
    dbgln("Current RTC time: " + String(now.year()) + "-" + String(now.month()) + "-" + String(now.day()) + " " + String(now.hour()) + ":" + String(now.minute()));
    
    int currentDayOfYear = 0;
    int daysInMonth[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    for (int i = 0; i < now.month() - 1; i++) {
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
    
    // Create cache with today's schedule
    cachedSchedulesDoc = new DynamicJsonDocument(512); // Much smaller buffer
    JsonArray todaySchedules = cachedSchedulesDoc->createNestedArray("schedules");
    
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
        if (buffer.length() > 100) {
            buffer = buffer.substring(buffer.length() - 50); // Keep last 50 chars
        }
        
        if (c == '[' && buffer.indexOf("\"schedules\"") != -1) {
            inSchedulesArray = true;
            dbgln("Found schedules array start");
            continue;
        }
        
        if (inSchedulesArray && c == '{') {
            bracketCount++;
            inScheduleObject = true;
            currentSchedule = "{";
            continue;
        }
        
        if (inScheduleObject) {
            currentSchedule += c;
            
            if (c == '{') bracketCount++;
            if (c == '}') bracketCount--;
            
            // Complete schedule object found
            if (bracketCount == 0 && c == '}') {
                processedSchedules++;
                
                // Parse this single schedule
                DynamicJsonDocument scheduleDoc(200);
                DeserializationError error = deserializeJson(scheduleDoc, currentSchedule);
                
                if (!error) {
                    int scheduleDay = scheduleDoc["d"].as<int>();
                    
                    if (processedSchedules <= 5 || scheduleDay == currentDayOfYear) {
                        dbgln("Schedule " + String(processedSchedules) + ": Day " + String(scheduleDay) + " vs target " + String(currentDayOfYear));
                    }
                    
                    if (scheduleDay == currentDayOfYear) {
                        // Found today's schedule!
                        JsonObject todaySchedule = todaySchedules.createNestedObject();
                        todaySchedule["d"] = scheduleDoc["d"];
                        todaySchedule["on"] = scheduleDoc["on"];
                        todaySchedule["off"] = scheduleDoc["off"];
                        // Enable field removed - all schedules are always active
                        foundToday = true;
                        
                        dbgln("SUCCESS: Found today's schedule: " + scheduleDoc["on"].as<String>() + " to " + scheduleDoc["off"].as<String>());
                        break;
                    }
                }
                
                inScheduleObject = false;
                currentSchedule = "";
            }
        }
        
        // Stop if we've processed enough schedules (optimization)
        if (processedSchedules > currentDayOfYear + 10) {
            dbgln("Stopping search after " + String(processedSchedules) + " schedules");
            break;
        }
    }
    
    file.close();
    
    dbgln("Processed " + String(processedSchedules) + " schedules via streaming");
    
    if (!foundToday) {
        dbgln("WARNING: No schedules found for today (day " + String(currentDayOfYear) + ")");
        dbgln("Using fallback schedule: Day 1 (January 1) - ON at 18:00, OFF at 06:00");
        
        // Create fallback schedule for day 1
        JsonObject fallbackSchedule = todaySchedules.createNestedObject();
        fallbackSchedule["d"] = currentDayOfYear;  // January 1st
        fallbackSchedule["on"] = "18:00";
        fallbackSchedule["off"] = "06:00";
        foundToday = true;
    }
    
    schedulesCacheValid = true;
    cachedDayOfYear = currentDayOfYear;
    dbgln("Today's schedules loaded to cache successfully (streaming approach)");
}

// Function to initialize schedules cache (call in setup)
void initSchedulesCache()
{
    loadTodaysSchedulesToCache();
}

void checkSchedules()
{
    dbgln("=== Checking schedules ===");
    
    // Get current time first
    DateTime now = rtc.now();
    int currentYear = now.year();
    dbgln("Current RTC time: " + String(now.year()) + "-" + String(now.month()) + "-" + String(now.day()) + " " + String(now.hour()) + ":" + String(now.minute()));
    
    if (currentYear < 2025 || currentYear > 2080)
    {
        dbgln("ERROR: Invalid year detected: " + String(currentYear) + ". Skipping schedule check.");
        return;
    }

    // Calculate current day of year
    int currentDayOfYear = 0;
    int daysInMonth[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; // Using 29 for Feb to handle leap years
    
    for (int i = 0; i < now.month() - 1; i++) {
        currentDayOfYear += daysInMonth[i];
    }
    currentDayOfYear += now.day();
    
    dbgln("Calculated day of year: " + String(currentDayOfYear));
    dbgln("Cache status - Valid: " + String(schedulesCacheValid ? "true" : "false") + ", Doc: " + String(cachedSchedulesDoc != nullptr ? "exists" : "null") + ", Cached day: " + String(cachedDayOfYear));
    // Check if cache is valid and for today
    if (!schedulesCacheValid || cachedSchedulesDoc == nullptr || cachedDayOfYear != currentDayOfYear)
    {
        dbgln("Cache invalid or day changed, reloading...");
        loadTodaysSchedulesToCache();
        return; // No valid schedules to check
    }
    
    dbgln("Cache is valid, proceeding with schedule check");

    // Format current time as HH:MM
    String currentTime = "";
    if (now.hour() < 10)
        currentTime += "0";
    currentTime += String(now.hour());
    currentTime += ":";
    if (now.minute() < 10)
        currentTime += "0";
    currentTime += String(now.minute());

    dbgln("Current day of year: " + String(currentDayOfYear) + " Current time: " + currentTime);
    
    // Check each schedule using cached document
    JsonArray schedules = (*cachedSchedulesDoc)["schedules"];
    for (JsonObject schedule : schedules)
    {
        // All schedules are always active - no enable check needed

        // Check if current day of year matches schedule day of year
        int scheduleDayOfYear = schedule["d"];
        if (scheduleDayOfYear != currentDayOfYear)
        {
            continue; // Day of year doesn't match
        }

        // Print schedule information for today
        dbgln("=== Today's Schedule Found ===");
        dbgln("Day of year: " + String(scheduleDayOfYear));
        dbgln("ON time: " + String(schedule["on"].as<String>()));
        dbgln("OFF time: " + String(schedule["off"].as<String>()));

        // Get ON and OFF times
        const char *onTime = schedule["on"];
        const char *offTime = schedule["off"];
        
        // Convert times to minutes for easier comparison
        int onMinutes = ((onTime[0] - '0') * 10 + (onTime[1] - '0')) * 60 + 
                       ((onTime[3] - '0') * 10 + (onTime[4] - '0'));
        int offMinutes = ((offTime[0] - '0') * 10 + (offTime[1] - '0')) * 60 + 
                        ((offTime[3] - '0') * 10 + (offTime[4] - '0'));
        int currentMinutes = now.hour() * 60 + now.minute();
        
        // Determine which time comes first (first) and which comes second (second)
        int firstTime, secondTime;
        const char *firstTimeStr, *secondTimeStr;
        
        if (onMinutes < offMinutes)
        {
            // ON comes first, OFF comes second
            firstTime = onMinutes;
            secondTime = offMinutes;
            firstTimeStr = onTime;
            secondTimeStr = offTime;
        }
        else
        {
            // OFF comes first, ON comes second
            firstTime = offMinutes;
            secondTime = onMinutes;
            firstTimeStr = offTime;
            secondTimeStr = onTime;
        }
        
        // Apply time-range logic
        if (currentMinutes < firstTime && currentMinutes < secondTime)
        {
            // Current time is before both times - apply the state of the second time
            if (onMinutes < offMinutes)
            {
                // ON comes first, OFF comes second - so before both means OFF state
                dbgln("Time before schedule: " + String(onTime) + " to " + String(offTime) + " - Turning OFF");
                bulb.off();
            }
            else
            {
                // OFF comes first, ON comes second - so before both means ON state
                dbgln("Time before schedule: " + String(offTime) + " to " + String(onTime) + " - Turning ON");
                bulb.on();
            }
        }
        else if (currentMinutes > firstTime && currentMinutes > secondTime)
        {
            // Current time is after both times - apply the state of the second time
            if (onMinutes < offMinutes)
            {
                // ON comes first, OFF comes second - so after both means OFF state
                dbgln("Time after schedule: " + String(onTime) + " to " + String(offTime) + " - Turning OFF");
                bulb.off();
            }
            else
            {
                // OFF comes first, ON comes second - so after both means ON state
                dbgln("Time after schedule: " + String(offTime) + " to " + String(onTime) + " - Turning ON");
                bulb.on();
            }
        }
        else if (currentMinutes >= firstTime && currentMinutes <= secondTime)
        {
            // Current time is between the two times
            if (onMinutes < offMinutes)
            {
                // ON comes first, so we're in the ON period
                dbgln("Time within ON period: " + String(onTime) + " to " + String(offTime) + " - Turning ON");
                bulb.on();
            }
            else
            {
                // OFF comes first, so we're in the OFF period
                dbgln("Time within OFF period: " + String(offTime) + " to " + String(onTime) + " - Turning OFF");
                bulb.off();
            }
        }
        
        // Only process the first matching schedule for the day
        break;
    }
}

void controlDevices()
{
    led.loop();
    bulb.loop();
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
