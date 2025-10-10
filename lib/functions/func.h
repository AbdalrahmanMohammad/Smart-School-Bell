// Global variable to track last triggered time to prevent multiple triggers
String lastTriggeredTime = "";

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
    // Free existing cache if it exists
    if (cachedSchedulesDoc != nullptr)
    {
        delete cachedSchedulesDoc;
        cachedSchedulesDoc = nullptr;
    }

    // Get current day of year
    DateTime now = rtc.now();
    int currentDayOfYear = 0;
    int daysInMonth[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    
    for (int i = 0; i < now.month() - 1; i++) {
        currentDayOfYear += daysInMonth[i];
    }
    currentDayOfYear += now.day();

    // Read schedules from flash memory using streaming approach
    File file = LittleFS.open("/schedules.txt", "r");
    if (!file)
    {
        // Fallback to JSON format if text file doesn't exist
        dbgln("schedules.txt not found, trying schedules.json...");
        file = LittleFS.open("/schedules.json", "r");
        if (!file)
        {
            schedulesCacheValid = false;
            dbgln("No schedules file found");
            return; // No schedules file
        }
        
        // Use JSON parsing for fallback
        String jsonData = file.readString();
        file.close();
        
        DynamicJsonDocument fullDoc(30000);
        DeserializationError error = deserializeJson(fullDoc, jsonData);
        
        if (error)
        {
            dbgln("Failed to parse schedules.json: " + String(error.c_str()));
            schedulesCacheValid = false;
            return;
        }
        
        // Create cache with today's schedule
        cachedSchedulesDoc = new DynamicJsonDocument(1024);
        JsonArray todaySchedules = cachedSchedulesDoc->createNestedArray("schedules");
        
        JsonArray allSchedules = fullDoc["schedules"];
        bool foundToday = false;
        
        for (JsonObject schedule : allSchedules)
        {
            if (schedule["d"].as<int>() == currentDayOfYear)
            {
                JsonObject todaySchedule = todaySchedules.createNestedObject();
                todaySchedule["d"] = schedule["d"];
                todaySchedule["on"] = schedule["on"];
                todaySchedule["off"] = schedule["off"];
                todaySchedule["e"] = schedule["e"];
                foundToday = true;
                dbgln("Found today's schedule: " + schedule["on"].as<String>() + " to " + schedule["off"].as<String>() + " (bulb)");
                break;
            }
        }
        
        if (!foundToday) {
            dbgln("No schedules found for today (day " + String(currentDayOfYear) + ")");
        }
        
        schedulesCacheValid = true;
        cachedDayOfYear = currentDayOfYear;
        dbgln("Today's schedules loaded to cache successfully (JSON fallback)");
        return;
    }

    // Create a new document with only today's schedules
    cachedSchedulesDoc = new DynamicJsonDocument(1024); // Small cache for just today
    JsonArray todaySchedules = cachedSchedulesDoc->createNestedArray("schedules");
    
    bool foundToday = false;
    String line;
    
    // Read line by line to find today's schedule
    while (file.available()) {
        line = file.readStringUntil('\n');
        line.trim();
        
        if (line.length() == 0) continue;
        
        // Parse: d,on,off,e (optimized format)
        int firstComma = line.indexOf(',');
        int secondComma = line.indexOf(',', firstComma + 1);
        int thirdComma = line.indexOf(',', secondComma + 1);
        
        if (firstComma == -1 || secondComma == -1 || thirdComma == -1) {
            continue; // Skip malformed lines
        }
        
        int dayOfYear = line.substring(0, firstComma).toInt();
        
        if (dayOfYear == currentDayOfYear) {
            // Found today's schedule!
            String onTime = line.substring(firstComma + 1, secondComma);
            String offTime = line.substring(secondComma + 1, thirdComma);
            bool enabled = line.substring(thirdComma + 1).toInt() == 1;
            
            JsonObject todaySchedule = todaySchedules.createNestedObject();
            todaySchedule["d"] = dayOfYear;
            todaySchedule["on"] = onTime;
            todaySchedule["off"] = offTime;
            todaySchedule["e"] = enabled;
            foundToday = true;
            
            dbgln("Found today's schedule: " + onTime + " to " + offTime + " (bulb)");
            break; // Found today's schedule, no need to continue reading
        }
    }
    
    file.close();
    
    if (!foundToday) {
        dbgln("No schedules found for today (day " + String(currentDayOfYear) + ")");
    }

    schedulesCacheValid = true;
    cachedDayOfYear = currentDayOfYear;
    dbgln("Today's schedules loaded to cache successfully");
}

// Function to initialize schedules cache (call in setup)
void initSchedulesCache()
{
    loadTodaysSchedulesToCache();
}

void checkSchedules()
{
    // Get current time first
    DateTime now = rtc.now();
    int currentYear = now.year();
    dbgln("Current year: " + String(currentYear));
    if (currentYear < 2025 || currentYear > 2060)
    {
        dbgln("Invalid year detected: " + String(currentYear) + ". Skipping schedule check.");
        return;
    }

    // Calculate current day of year
    int currentDayOfYear = 0;
    int daysInMonth[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; // Using 29 for Feb to handle leap years
    
    for (int i = 0; i < now.month() - 1; i++) {
        currentDayOfYear += daysInMonth[i];
    }
    currentDayOfYear += now.day();

    // Check if cache is valid and for today
    if (!schedulesCacheValid || cachedSchedulesDoc == nullptr || cachedDayOfYear != currentDayOfYear)
    {
        dbgln("Schedules cache invalid or day changed, reloading...");
        loadTodaysSchedulesToCache();
        return; // No valid schedules to check
    }

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
        if (!schedule["e"].as<bool>())
        {
            continue;
        }

        // Check if current day of year matches schedule day of year
        int scheduleDayOfYear = schedule["d"];
        if (scheduleDayOfYear != currentDayOfYear)
        {
            continue; // Day of year doesn't match
        }

        // Check if current time matches ON time
        const char *onTime = schedule["on"];
        if (strcmp(onTime, currentTime.c_str()) == 0)
        {
            // ON time matches! Turn on the bulb
            dbg("Turning ON bulb at scheduled time: ");
            dbgln(onTime);
            bulb.on();
            lastTriggeredTime = currentTime; // Mark this time as triggered
        }
        // Check if current time matches OFF time
        else
        {
            const char *offTime = schedule["off"];
            if (strcmp(offTime, currentTime.c_str()) == 0)
            {
                // OFF time matches! Turn off the bulb
                dbg("Turning OFF bulb at scheduled time: ");
                dbgln(offTime);
                bulb.off();
                lastTriggeredTime = currentTime; // Mark this time as triggered
            }
        }
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
