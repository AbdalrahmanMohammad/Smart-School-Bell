
void handleRoot()
{
    File file = LittleFS.open("/index.html", "r");
    if (!file)
    {
        server.send(500, "text/plain", "Failed to open file");
        return;
    }
    server.streamFile(file, "text/html");
    file.close();
}

void handleCSS()
{
    File file = LittleFS.open("/style.css", "r");
    if (!file)
    {
        server.send(404, "text/plain", "CSS file not found");
        return;
    }
    server.streamFile(file, "text/css");
    file.close();
}

void handleJS()
{
    File file = LittleFS.open("/script.js", "r");
    if (!file)
    {
        server.send(404, "text/plain", "JavaScript file not found");
        return;
    }
    server.streamFile(file, "application/javascript");
    file.close();
}

void handleTime()
{
    DateTime now = rtc.now();
    String timeString = "";

    // Format: YYYY/MM/DD HH:MM:SS
    timeString += now.year();
    timeString += "/";
    if (now.month() < 10)
        timeString += "0";
    timeString += now.month();
    timeString += "/";
    if (now.day() < 10)
        timeString += "0";
    timeString += now.day();
    timeString += " ";
    if (now.hour() < 10)
        timeString += "0";
    timeString += now.hour();
    timeString += ":";
    if (now.minute() < 10)
        timeString += "0";
    timeString += now.minute();
    timeString += ":";
    if (now.second() < 10)
        timeString += "0";
    timeString += now.second();

    server.send(200, "text/plain", timeString);
}

void handleStatus()
{
    StaticJsonDocument<200> doc;
    doc["led"] = led.isOn();
    doc["bell"] = bell.isOn();

    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

void handleLEDToggle()
{
    led.toggle();

    StaticJsonDocument<100> doc;
    doc["led"] = led.isOn();

    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

// ===== Config helpers =====
static const char *CONFIG_PATH = "/config.json";

static void loadConfigOrDefaults(StaticJsonDocument<256> &cfg)
{
    cfg.clear();
    File f = LittleFS.open(CONFIG_PATH, "r");
    if (!f)
    {
        // defaults
        cfg["bellDurationMs"] = 3000; // 3s default
        cfg["ledOn"] = false;
        return;
    }
    String content = f.readString();
    f.close();
    DeserializationError err = deserializeJson(cfg, content);
    if (err)
    {
        cfg.clear();
        cfg["bellDurationMs"] = 3000;
        cfg["ledOn"] = false;
    }
}

static bool saveConfig(const StaticJsonDocument<256> &cfg)
{
    File f = LittleFS.open(CONFIG_PATH, "w");
    if (!f)
        return false;
    serializeJson(cfg, f);
    f.close();
    return true;
}

void handleGetConfig()
{
    StaticJsonDocument<256> cfg;
    loadConfigOrDefaults(cfg);
    String json;
    serializeJson(cfg, json);
    server.send(200, "application/json", json);
}

void handleUpdateBellDuration()
{
    if (!server.hasArg("plain"))
    {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"No data\"}");
        return;
    }

    StaticJsonDocument<128> body;
    DeserializationError err = deserializeJson(body, server.arg("plain"));
    if (err)
    {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"Bad JSON\"}");
        return;
    }

    // Accept either bellDurationMs or seconds
    unsigned long bellDurationMs = 0;
    if (body.containsKey("bellDurationMs"))
    {
        bellDurationMs = body["bellDurationMs"].as<unsigned long>();
    }
    else if (body.containsKey("bellDurationSeconds"))
    {
        bellDurationMs = body["bellDurationSeconds"].as<unsigned long>() * 1000UL;
    }
    else
    {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"Missing duration\"}");
        return;
    }

    // Clamp to a reasonable upper bound (e.g., 60s)
    if (bellDurationMs > 60000UL)
        bellDurationMs = 60000UL;

    // Persist
    StaticJsonDocument<256> cfg;
    loadConfigOrDefaults(cfg);
    cfg["bellDurationMs"] = bellDurationMs;
    saveConfig(cfg);

    // Apply immediately
    bell.setDuration(bellDurationMs);

    server.send(200, "application/json", "{\"success\":true}");
}

void handleBellToggle()
{
    // Print file content
    File file = LittleFS.open("/schedules.json", "r");
    if (!file)
    {
        dbg("Schedules file not found, assuming no schedules");
    }
    else
    {
        String jsonData = file.readString();
        dbgln("File schedules: " + jsonData);
        file.close();
    }

    // Print separator
    dbgln("***********");

    // Print cached content
    if (!schedulesCacheValid || cachedSchedulesDoc == nullptr)
    {
        loadSchedulesToCache();
    }

    if (schedulesCacheValid && cachedSchedulesDoc != nullptr)
    {
        String cachedJsonData;
        serializeJson(*cachedSchedulesDoc, cachedJsonData);
        dbgln("Cached schedules: " + cachedJsonData);
    }
    else
    {
        dbgln("No cached schedules available");
    }

    bell.on();

    StaticJsonDocument<100> doc;
    doc["bell"] = bell.isOn();
    dbgln("--------------------------");
    String json;
    serializeJson(doc, json);
    server.send(200, "application/json", json);
}

void handleSchedules()
{
    File file = LittleFS.open("/schedules.json", "r");
    if (!file)
    {
        server.send(200, "application/json", "{\"schedules\":[]}");
        return;
    }

    String json = file.readString();
    file.close();
    server.send(200, "application/json", json);
}

void handleAddSchedule()
{
    if (server.hasArg("plain"))
    {
        String jsonData = server.arg("plain");
        dbgln("Adding new schedule...");

        // Parse the new schedule
        DynamicJsonDocument newScheduleDoc(1024);
        DeserializationError error = deserializeJson(newScheduleDoc, jsonData);

        if (error)
        {
            dbgln("Error: Failed to parse schedule JSON");
            server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON format\"}");
            return;
        }

        delete cachedSchedulesDoc; // Free existing cache
        cachedSchedulesDoc = nullptr;
        // Read current schedules
        File file = LittleFS.open("/schedules.json", "r");
        DynamicJsonDocument schedulesDoc(16384); // 16KB for large number of schedules

        if (file)
        {
            String currentJson = file.readString();
            file.close();

            error = deserializeJson(schedulesDoc, currentJson);
            if (error)
            {
                dbgln("Warning: Corrupted schedules file, creating fresh");
                schedulesDoc.clear();
                schedulesDoc.createNestedArray("schedules");
            }
        }
        else
        {
            // File doesn't exist, create new structure
            schedulesDoc.createNestedArray("schedules");
        }

        // Check alarm limit (50 alarms maximum)
        JsonArray schedules = schedulesDoc["schedules"];
        if (schedules.size() >= 50)
        {
            dbgln("Error: Maximum number of alarms (50) reached");
            server.send(400, "application/json", "{\"success\":false,\"message\":\"Maximum number of alarms (50) reached. Please delete some alarms before adding new ones.\"}");
            return;
        }

        // Add the new schedule
        JsonObject newSchedule = schedules.createNestedObject();

        // Copy all fields from the new schedule
        for (JsonPair kv : newScheduleDoc.as<JsonObject>())
        {
            newSchedule[kv.key()] = kv.value();
        }

        // Write back to file
        File writeFile = LittleFS.open("/schedules.json", "w");
        if (writeFile)
        {
            serializeJson(schedulesDoc, writeFile);
            writeFile.close();

            dbgln("Schedule added successfully");
            // loadSchedulesToCache(); // not necessary, check schedules will do it since cachedSchedulesDoc == nullptr
            server.send(200, "application/json", "{\"success\":true}");
        }
        else
        {
            dbgln("Error: Failed to write schedules file");
            server.send(500, "application/json", "{\"success\":false,\"message\":\"Failed to write file\"}");
        }
    }
    else
    {
        dbgln("Error: No schedule data received");
        server.send(400, "application/json", "{\"success\":false,\"message\":\"No data received\"}");
    }
}

void handleDeleteSchedule()
{
    dbgln("Delete request received");

    // Check if we have POST data
    if (!server.hasArg("plain"))
    {
        dbgln("Error: No POST data");
        server.send(400, "application/json", "{\"success\":false}");
        return;
    }

    // Get and parse the request data
    String jsonData = server.arg("plain");
    dbg("Delete request data: ");
    dbgln(jsonData);

    StaticJsonDocument<64> requestDoc;
    DeserializationError error = deserializeJson(requestDoc, jsonData);

    if (error)
    {
        dbg("Parse error: ");
        dbgln(error.c_str());
        server.send(400, "application/json", "{\"success\":false}");
        return;
    }

    // Get the index to delete
    int index = requestDoc["index"];
    dbg("Index to delete: ");
    dbgln(index);

    // Open and read schedules file
    File file = LittleFS.open("/schedules.json", "r");
    if (!file)
    {
        dbgln("Error: File not found");
        server.send(404, "application/json", "{\"success\":false}");
        return;
    }

    String fileContent = file.readString();
    file.close();
    dbg("File size: ");
    dbgln(fileContent.length());

    delete cachedSchedulesDoc; // Free existing cache
    cachedSchedulesDoc = nullptr;

    // Parse schedules
    DynamicJsonDocument schedulesDoc(16384); // 16KB for large number of schedules
    error = deserializeJson(schedulesDoc, fileContent);

    if (error)
    {
        dbg("Schedule parse error: ");
        dbgln(error.c_str());
        server.send(400, "application/json", "{\"success\":false}");
        return;
    }

    // Get schedules array and validate index
    JsonArray schedules = schedulesDoc["schedules"];
    int count = schedules.size();
    dbg("Total schedules: ");
    dbgln(count);

    if (index < 0 || index >= count)
    {
        dbgln("Error: Invalid index");
        server.send(400, "application/json", "{\"success\":false}");
        return;
    }

    // Remove the schedule and write back to file
    schedules.remove(index);
    dbgln("Schedule removed from array");

    File writeFile = LittleFS.open("/schedules.json", "w");
    if (!writeFile)
    {
        dbgln("Error: Failed to open file for writing");
        server.send(500, "application/json", "{\"success\":false}");
        return;
    }

    serializeJson(schedulesDoc, writeFile);
    writeFile.close();

    dbgln("Schedule deleted successfully");
    // loadSchedulesToCache(); // not necessary, check schedules will do it since cachedSchedulesDoc == nullptr
    server.send(200, "application/json", "{\"success\":true}");
}

void handleEditSchedule()
{
    // Check if we have POST data
    if (!server.hasArg("plain"))
    {
        dbgln("Error: No edit data received");
        server.send(400, "application/json", "{\"success\":false}");
        return;
    }

    // Get and parse the request data
    String jsonData = server.arg("plain");
    StaticJsonDocument<256> requestDoc;
    DeserializationError error = deserializeJson(requestDoc, jsonData);

    if (error)
    {
        dbgln("Error: Failed to parse edit request");
        server.send(400, "application/json", "{\"success\":false}");
        return;
    }

    // Get the edit data
    int index = requestDoc["index"];
    const char *newTime = requestDoc["time"];
    JsonArray newDays = requestDoc["days"];
    bool newEnabled = requestDoc["enabled"];

    // Open and read schedules file
    File file = LittleFS.open("/schedules.json", "r");
    if (!file)
    {
        dbgln("Error: Schedules file not found");
        server.send(404, "application/json", "{\"success\":false}");
        return;
    }

    String fileContent = file.readString();
    file.close();

    delete cachedSchedulesDoc; // Free existing cache
    cachedSchedulesDoc = nullptr;
    // Parse schedules
    DynamicJsonDocument schedulesDoc(16384); // 16KB for large number of schedules
    error = deserializeJson(schedulesDoc, fileContent);

    if (error)
    {
        dbgln("Error: Failed to parse schedules file");
        server.send(400, "application/json", "{\"success\":false}");
        return;
    }

    // Get schedules array and validate index
    JsonArray schedules = schedulesDoc["schedules"];
    if (index < 0 || index >= (int)schedules.size())
    {
        dbgln("Error: Invalid schedule index");
        server.send(400, "application/json", "{\"success\":false}");
        return;
    }

    // Update the schedule
    JsonObject schedule = schedules[index];

    // Update time
    schedule["time"] = newTime;

    // Update days array
    schedule.remove("days");
    JsonArray daysArray = schedule.createNestedArray("days");
    for (JsonVariant day : newDays)
    {
        daysArray.add(day.as<int>());
    }

    // Update enabled status
    schedule["enabled"] = newEnabled;

    // Write back to file
    File writeFile = LittleFS.open("/schedules.json", "w");
    if (!writeFile)
    {
        dbgln("Error: Failed to write schedules file");
        server.send(500, "application/json", "{\"success\":false}");
        return;
    }

    serializeJson(schedulesDoc, writeFile);
    writeFile.close();

    dbgln("Schedule edited successfully");
    // loadSchedulesToCache(); // not necessary, check schedules will do it since cachedSchedulesDoc == nullptr
    server.send(200, "application/json", "{\"success\":true}");
}

void handleSendTime()
{
    // Check if we have POST data
    if (!server.hasArg("plain"))
    {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"No data received\"}");
        return;
    }

    // Get the raw JSON data
    String jsonData = server.arg("plain");

    // Parse the JSON data
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, jsonData);

    if (error)
    {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON format\"}");
        return;
    }

    // Extract time data
    const char *timeString = doc["time"];

    // Parse ISO time string directly
    // Format: "2025-09-01T13:30:55" (local timezone format)
    if (strlen(timeString) < 19)
    {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid time format\"}");
        return;
    }

    // Extract year, month, day, hour, minute, second from ISO string
    int year = atoi(timeString);
    int month = atoi(timeString + 5);
    int day = atoi(timeString + 8);
    int hour = atoi(timeString + 11);
    int minute = atoi(timeString + 14);
    int second = atoi(timeString + 17);

    // Create DateTime object
    DateTime newTime(year, month, day, hour, minute, second);

    // Set the RTC
    rtc.adjust(newTime);

    // Send success response
    StaticJsonDocument<100> response;
    response["success"] = true;
    response["message"] = "RTC time set successfully";

    String responseJson;
    serializeJson(response, responseJson);

    server.send(200, "application/json", responseJson);
}

void WifiSetup()
{
    // Configure as Access Point
    const char *ssid = "NodeMCU_AP";
    const char *password = "12345678"; // at least 8 chars

    WiFi.softAP(ssid, password);

    dbgln("Access Point Started");
    dbg("IP address: ");
    dbgln(WiFi.softAPIP());

    // Setup web server routes
    server.on("/", handleRoot);
    server.on("/style.css", handleCSS);
    server.on("/script.js", handleJS);
    server.on("/time", handleTime);
    server.on("/status", handleStatus);
    server.on("/led/toggle", HTTP_POST, handleLEDToggle);
    server.on("/bell/toggle", HTTP_POST, handleBellToggle);
    server.on("/schedules", handleSchedules);
    server.on("/schedules/add", HTTP_POST, handleAddSchedule);
    server.on("/schedules/delete", HTTP_POST, handleDeleteSchedule);
    server.on("/schedules/edit", HTTP_POST, handleEditSchedule); // Added edit route
    server.on("/send-time", HTTP_POST, handleSendTime);          // Added send-time route

    // Config endpoints
    server.on("/config", handleGetConfig);
    server.on("/config/bell-duration", HTTP_POST, handleUpdateBellDuration);
    server.begin();
    dbgln("Web server started");
}