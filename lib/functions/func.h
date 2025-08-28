
void handleRoot()
{
    String html = R"rawliteral(
  <!DOCTYPE html>
  <html lang="en">
  <head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>NodeMCU Access Point</title>
    <style>
      body {
        font-family: Arial, sans-serif;
        background: linear-gradient(to right, #6a11cb, #2575fc);
        color: white;
        text-align: center;
        padding: 50px;
      }
      h1 {
        font-size: 2.5em;
        margin-bottom: 20px;
      }
      p {
        font-size: 1.2em;
      }
      a.button {
        display: inline-block;
        margin-top: 30px;
        padding: 15px 30px;
        font-size: 1.2em;
        color: #2575fc;
        background: white;
        border-radius: 8px;
        text-decoration: none;
        transition: 0.3s;
      }
      a.button:hover {
        background: #f0f0f0;
      }
      footer {
        margin-top: 50px;
        font-size: 0.9em;
        opacity: 0.8;
      }
    </style>
  </head>
  <body>
    <h1>Welcome to NodeMCU AP</h1>
    <p>You are now connected to the NodeMCU Access Point.</p>
    <a class="button" href="/">Refresh</a>
    <footer>NodeMCU Web Server &copy; 2025</footer>
  </body>
  </html>
  )rawliteral";

    server.send(200, "text/html", html);
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