
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
    if (now.month() < 10) timeString += "0";
    timeString += now.month();
    timeString += "/";
    if (now.day() < 10) timeString += "0";
    timeString += now.day();
    timeString += " ";
    if (now.hour() < 10) timeString += "0";
    timeString += now.hour();
    timeString += ":";
    if (now.minute() < 10) timeString += "0";
    timeString += now.minute();
    timeString += ":";
    if (now.second() < 10) timeString += "0";
    timeString += now.second();
    
    server.send(200, "text/plain", timeString);
}

void handleStatus()
{
    String json = "{\"led\":" + String(led.isOn() ? "true" : "false") + 
                  ",\"bell\":" + String(bell.isOn() ? "true" : "false") + "}";
    server.send(200, "application/json", json);
}

void handleLEDToggle()
{
    led.toggle();
    String json = "{\"led\":" + String(led.isOn() ? "true" : "false") + "}";
    server.send(200, "application/json", json);
}

void handleBellToggle()
{
    bell.on();
    String json = "{\"bell\":" + String(bell.isOn() ? "true" : "false") + "}";
    server.send(200, "application/json", json);
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
    server.begin();
    Serial.println("Web server started");
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