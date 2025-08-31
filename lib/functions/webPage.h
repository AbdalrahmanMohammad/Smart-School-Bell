
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

void handleBellToggle()
{
    File file = LittleFS.open("/schedules.json", "r");
    if (!file)
    {
        Serial.print("Schedules file not found, assuming no schedules");
        server.send(200, "application/json", "{\"schedules\":[]}");
        return;
    }
    
    String jsonData = file.readString();
    Serial.println("Current schedules: " + jsonData);
    file.close();
    
    bell.on();
    
    StaticJsonDocument<100> doc;
    doc["bell"] = bell.isOn();
    
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
    Serial.println("=== handleAddSchedule() START ===");
    
    if (server.hasArg("plain"))
    {
        String jsonData = server.arg("plain");
        Serial.println("✅ Step 1: Received POST data");
        Serial.print("📥 Raw JSON data: ");
        Serial.println(jsonData);
        
        // Parse the new schedule
        Serial.println("🔄 Step 2: Parsing new schedule JSON");
        StaticJsonDocument<1024> newScheduleDoc;
        DeserializationError error = deserializeJson(newScheduleDoc, jsonData);
        
        if (error)
        {
            Serial.print("❌ Step 2 FAILED: Failed to parse new schedule - ");
            Serial.println(error.c_str());
            server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON format\"}");
            Serial.println("=== handleAddSchedule() END (ERROR) ===");
            return;
        }
        Serial.println("✅ Step 2: New schedule parsed successfully");
        
        // Debug: Print the parsed schedule
        Serial.print("📋 Parsed schedule contents: ");
        serializeJson(newScheduleDoc, Serial);
        Serial.println();
        
        // Read current schedules
        Serial.println("🔄 Step 3: Reading existing schedules file");
        File file = LittleFS.open("/schedules.json", "r");
        StaticJsonDocument<4096> schedulesDoc;
        
        if (file)
        {
            Serial.println("✅ Step 3a: Existing schedules file found");
            String currentJson = file.readString();
            file.close();
            Serial.print("📄 Current file contents: ");
            Serial.println(currentJson);
            
            Serial.println("🔄 Step 3b: Parsing existing schedules");
            error = deserializeJson(schedulesDoc, currentJson);
            if (error)
            {
                Serial.print("⚠️ Step 3b WARNING: Failed to parse existing schedules - ");
                Serial.println(error.c_str());
                Serial.println("🔄 Creating fresh schedules structure");
                // If existing file is corrupted, start fresh
                schedulesDoc.clear();
                schedulesDoc.createNestedArray("schedules");
            }
            else
            {
                Serial.println("✅ Step 3b: Existing schedules parsed successfully");
            }
        }
        else
        {
            Serial.println("ℹ️ Step 3a: No existing schedules file found, creating new structure");
            // File doesn't exist, create new structure
            schedulesDoc.createNestedArray("schedules");
        }
        
        // Add the new schedule
        Serial.println("🔄 Step 4: Adding new schedule to array");
        JsonArray schedules = schedulesDoc["schedules"];
        Serial.print("📊 Current schedules count: ");
        Serial.println(schedules.size());
        
        JsonObject newSchedule = schedules.createNestedObject();
        Serial.println("✅ Step 4a: Created new schedule object in array");
        
        // Copy all fields from the new schedule
        Serial.println("🔄 Step 4b: Copying fields from new schedule");
        int fieldCount = 0;
        for (JsonPair kv : newScheduleDoc.as<JsonObject>())
        {
            Serial.print("📝 Copying field: ");
            Serial.print(kv.key().c_str());
            Serial.print(" = ");
            serializeJson(kv.value(), Serial);
            Serial.println();
            
            newSchedule[kv.key()] = kv.value();
            fieldCount++;
        }
        Serial.print("✅ Step 4b: Copied ");
        Serial.print(fieldCount);
        Serial.println(" fields");
        
        Serial.print("📊 Final schedules count: ");
        Serial.println(schedules.size());
        
        // Write back to file
        Serial.println("🔄 Step 5: Writing updated schedules to file");
        File writeFile = LittleFS.open("/schedules.json", "w");
        if (writeFile)
        {
            Serial.println("✅ Step 5a: File opened for writing");
            String updatedJson;
            serializeJson(schedulesDoc, updatedJson);
            Serial.print("📄 Final JSON to write: ");
            Serial.println(updatedJson);
            
            writeFile.print(updatedJson);
            writeFile.close();
            Serial.println("✅ Step 5b: File written and closed successfully");
            
            // Create response
            Serial.println("🔄 Step 6: Creating success response");
            StaticJsonDocument<100> response;
            response["success"] = true;
            
            String responseJson;
            serializeJson(response, responseJson);
            Serial.print("📤 Sending response: ");
            Serial.println(responseJson);
            
            server.send(200, "application/json", responseJson);
            Serial.println("✅ Step 6: Response sent successfully");
        }
        else
        {
            Serial.println("❌ Step 5 FAILED: Failed to open schedules file for writing");
            server.send(500, "application/json", "{\"success\":false,\"message\":\"Failed to write file\"}");
        }
    }
    else
    {
        Serial.println("❌ Step 1 FAILED: No data received in POST request");
        server.send(400, "application/json", "{\"success\":false,\"message\":\"No data received\"}");
    }
    
    Serial.println("=== handleAddSchedule() END ===");
}

void handleDeleteSchedule()
{
    Serial.println("=== handleDeleteSchedule() START ===");
    
    // Check if we have POST data
    if (!server.hasArg("plain"))
    {
        Serial.println("❌ No POST data");
        server.send(400, "application/json", "{\"success\":false}");
        return;
    }
    
    Serial.println("✅ Step 1: Got POST data");
    
    // Get the raw data
    String jsonData = server.arg("plain");
    Serial.print("📥 Length: ");
    Serial.println(jsonData.length());
    
    // Parse the request with tiny buffer
    Serial.println("🔄 Step 2: Parse request");
    StaticJsonDocument<64> requestDoc;
    DeserializationError error = deserializeJson(requestDoc, jsonData);
    
    if (error)
    {
        Serial.println("❌ Parse failed");
        server.send(400, "application/json", "{\"success\":false}");
        return;
    }
    
    Serial.println("✅ Step 2: Parsed");
    
    // Get the index
    int index = requestDoc["index"];
    Serial.print("🎯 Index: ");
    Serial.println(index);
    
    // Open file
    Serial.println("🔄 Step 3: Open file");
    File file = LittleFS.open("/schedules.json", "r");
    if (!file)
    {
        Serial.println("❌ File not found");
        server.send(404, "application/json", "{\"success\":false}");
        return;
    }
    
    Serial.println("✅ Step 3: File opened");
    
    // Read file content
    String fileContent = file.readString();
    file.close();
    
    Serial.print("📏 Size: ");
    Serial.println(fileContent.length());
    
    // Parse schedules
    Serial.println("🔄 Step 4: Parse schedules");
    StaticJsonDocument<2048> schedulesDoc;
    error = deserializeJson(schedulesDoc, fileContent);
    
    if (error)
    {
        Serial.println("❌ Schedule parse failed");
        server.send(400, "application/json", "{\"success\":false}");
        return;
    }
    
    Serial.println("✅ Step 4: Schedules parsed");
    
    // Get schedules array
    JsonArray schedules = schedulesDoc["schedules"];
    int count = schedules.size();
    Serial.print("📊 Count: ");
    Serial.println(count);
    
    // Validate index
    if (index < 0 || index >= count)
    {
        Serial.println("❌ Invalid index");
        server.send(400, "application/json", "{\"success\":false}");
        return;
    }
    
    Serial.println("✅ Step 5: Index valid");
    
    // Remove the schedule
    Serial.println("🔄 Step 6: Remove schedule");
    schedules.remove(index);
    Serial.println("✅ Step 6: Removed");
    
    // Write back to file
    Serial.println("🔄 Step 7: Write file");
    File writeFile = LittleFS.open("/schedules.json", "w");
    if (!writeFile)
    {
        Serial.println("❌ Write failed");
        server.send(500, "application/json", "{\"success\":false}");
        return;
    }
    
    // Write directly to file
    serializeJson(schedulesDoc, writeFile);
    writeFile.close();
    
    Serial.println("✅ Step 7: Written");
    
    // Send response
    Serial.println("🔄 Step 8: Send response");
    server.send(200, "application/json", "{\"success\":true}");
    Serial.println("✅ Step 8: Sent");
    
    Serial.println("=== handleDeleteSchedule() END ===");
}

void handleEditSchedule()
{
    Serial.println("=== handleEditSchedule() START ===");
    
    // Check if we have POST data
    if (!server.hasArg("plain"))
    {
        Serial.println("❌ No POST data");
        server.send(400, "application/json", "{\"success\":false}");
        return;
    }
    
    Serial.println("✅ Step 1: Got POST data");
    
    // Get the raw data
    String jsonData = server.arg("plain");
    Serial.print("📥 Length: ");
    Serial.println(jsonData.length());
    
    // Parse the request with minimal buffer
    Serial.println("🔄 Step 2: Parse request");
    StaticJsonDocument<256> requestDoc;
    DeserializationError error = deserializeJson(requestDoc, jsonData);
    
    if (error)
    {
        Serial.println("❌ Parse failed");
        server.send(400, "application/json", "{\"success\":false}");
        return;
    }
    
    Serial.println("✅ Step 2: Parsed");
    
    // Get the edit data
    int index = requestDoc["index"];
    const char* newTime = requestDoc["time"];
    JsonArray newDays = requestDoc["days"];
    bool newEnabled = requestDoc["enabled"];
    
    Serial.print("🎯 Index: ");
    Serial.println(index);
    Serial.print("🕐 New time: ");
    Serial.println(newTime);
    Serial.print("📅 Days count: ");
    Serial.println(newDays.size());
    Serial.print("🔔 Enabled: ");
    Serial.println(newEnabled ? "true" : "false");
    
    // Open file
    Serial.println("🔄 Step 3: Open file");
    File file = LittleFS.open("/schedules.json", "r");
    if (!file)
    {
        Serial.println("❌ File not found");
        server.send(404, "application/json", "{\"success\":false}");
        return;
    }
    
    Serial.println("✅ Step 3: File opened");
    
    // Read file content
    String fileContent = file.readString();
    file.close();
    
    Serial.print("📏 Size: ");
    Serial.println(fileContent.length());
    
    // Parse schedules with smaller buffer
    Serial.println("🔄 Step 4: Parse schedules");
    StaticJsonDocument<1024> schedulesDoc;
    error = deserializeJson(schedulesDoc, fileContent);
    
    if (error)
    {
        Serial.println("❌ Schedule parse failed");
        server.send(400, "application/json", "{\"success\":false}");
        return;
    }
    
    Serial.println("✅ Step 4: Schedules parsed");
    
    // Get schedules array
    JsonArray schedules = schedulesDoc["schedules"];
    int count = schedules.size();
    Serial.print("📊 Count: ");
    Serial.println(count);
    
    // Validate index
    if (index < 0 || index >= count)
    {
        Serial.println("❌ Invalid index");
        server.send(400, "application/json", "{\"success\":false}");
        return;
    }
    
    Serial.println("✅ Step 5: Index valid");
    
    // Update the schedule
    Serial.println("🔄 Step 6: Update schedule");
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
    
    Serial.println("✅ Step 6: Updated");
    
    // Write back to file
    Serial.println("🔄 Step 7: Write file");
    File writeFile = LittleFS.open("/schedules.json", "w");
    if (!writeFile)
    {
        Serial.println("❌ Write failed");
        server.send(500, "application/json", "{\"success\":false}");
        return;
    }
    
    // Write directly to file
    serializeJson(schedulesDoc, writeFile);
    writeFile.close();
    
    Serial.println("✅ Step 7: Written");
    
    // Send response
    Serial.println("🔄 Step 8: Send response");
    server.send(200, "application/json", "{\"success\":true}");
    Serial.println("✅ Step 8: Sent");
    
    Serial.println("=== handleEditSchedule() END ===");
}

void handleSendTime()
{
    Serial.println("=== handleSendTime() START ===");
    
    // Check if we have POST data
    if (!server.hasArg("plain"))
    {
        Serial.println("❌ No POST data received");
        server.send(400, "application/json", "{\"success\":false,\"message\":\"No data received\"}");
        return;
    }
    
    Serial.println("✅ Step 1: Got POST data");
    
    // Get the raw JSON data
    String jsonData = server.arg("plain");
    Serial.print("📥 Raw JSON data: ");
    Serial.println(jsonData);
    
    // Parse the JSON data
    Serial.println("🔄 Step 2: Parsing JSON data");
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, jsonData);
    
    if (error)
    {
        Serial.print("❌ Step 2 FAILED: Failed to parse JSON - ");
        Serial.println(error.c_str());
        server.send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON format\"}");
        return;
    }
    
    Serial.println("✅ Step 2: JSON parsed successfully");
    
    // Extract time data
    const char* timeString = doc["time"];
    long timestamp = doc["timestamp"];
    
    Serial.print("🕐 Received time: ");
    Serial.println(timeString);
    Serial.print("⏰ Timestamp: ");
    Serial.println(timestamp);
    
    // Serial print the time information
    Serial.println("📡 === PHONE TIME RECEIVED ===");
    Serial.print("📱 Phone Time: ");
    Serial.println(timeString);
    Serial.print("🕒 Unix Timestamp: ");
    Serial.println(timestamp);
    Serial.println("📡 === END PHONE TIME ===");
    
    // Send success response
    Serial.println("🔄 Step 3: Sending success response");
    StaticJsonDocument<100> response;
    response["success"] = true;
    response["message"] = "Time received and printed to serial";
    
    String responseJson;
    serializeJson(response, responseJson);
    Serial.print("📤 Response: ");
    Serial.println(responseJson);
    
    server.send(200, "application/json", responseJson);
    Serial.println("✅ Step 3: Response sent successfully");
    
    Serial.println("=== handleSendTime() END ===");
}

void WifiSetup()
{
    // Configure as Access Point
    const char *ssid = "NodeMCU_AP";
    const char *password = "12345678"; // at least 8 chars

    WiFi.softAP(ssid, password);

    Serial.println("Access Point Started");
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());

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
    server.on("/send-time", HTTP_POST, handleSendTime); // Added send-time route
    server.begin();
    Serial.println("Web server started");
}