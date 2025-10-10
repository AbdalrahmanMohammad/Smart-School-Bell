
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
    doc["bulb"] = bulb.isOn();

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
// Config is now handled by LED class - no duplicate functions needed


// Bulb duration handler removed - bulb is simple on/off only

void handleBulbToggle()
{
  

    bulb.toggle();

    StaticJsonDocument<100> doc;
    doc["bulb"] = bulb.isOn();
    // dbgln("--------------------------");
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

    // Just return the raw file content - no parsing needed!
    String jsonData = file.readString();
    file.close();
    
    server.send(200, "application/json", jsonData);
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
    StaticJsonDocument<512> requestDoc;
    DeserializationError error = deserializeJson(requestDoc, jsonData);

    if (error)
    {
        dbgln("Error: Failed to parse edit request");
        server.send(400, "application/json", "{\"success\":false}");
        return;
    }

    // Get the edit data
    int index = requestDoc["index"];
    int newDayOfYear = requestDoc["d"];
    const char *newOnTime = requestDoc["on"];
    const char *newOffTime = requestDoc["off"];
    bool newEnabled = requestDoc["e"];

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
    DynamicJsonDocument schedulesDoc(10240); // 10KB should be enough for 50 schedules
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

    // Update day of year and times (optimized format)
    schedule["d"] = newDayOfYear;
    schedule["on"] = newOnTime;
    schedule["off"] = newOffTime;

    // Update enabled status
    schedule["e"] = newEnabled;

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
    server.on("/bulb/toggle", HTTP_POST, handleBulbToggle);
    server.on("/schedules", handleSchedules);
    server.on("/schedules/edit", HTTP_POST, handleEditSchedule); // Added edit route
    server.on("/send-time", HTTP_POST, handleSendTime);          // Added send-time route

    // Config endpoints removed - LED class handles config directly
    // Bulb duration endpoint removed - bulb is simple on/off only
    server.begin();
    dbgln("Web server started");
}