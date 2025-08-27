// float measure(int num)
// {
//     float sum = 0;
//     float measure;
//     int rounds = 0;
//     for (int i = 0; i < num; i++)
//     {
//         measure = ultrasonicSensor.measureDistanceCm();
//         if (measure >= 0)
//         {
//             sum += measure;
//             rounds++;
//         }
//         delay(3);
//     }
//     return sum / rounds;
// }

// void connect_wifi()
// {
//     WiFi.begin(ssid, password);

//     Serial.println("Connecting");
//     while (WiFi.status() != WL_CONNECTED)
//     {
//         delay(500);
//         Serial.print(".");
//     }
//     Serial.println("");
//     Serial.print("Connected to WiFi network with IP Address: ");
//     Serial.println(WiFi.localIP());
// }

// void checkWifi() // checks wifi connection every 3 seconds and shows the condition on wifiLed
// {
//     if (millis() - wifiPrevious >= 3000UL)
//     {
//         if (WiFi.status() == WL_CONNECTED)
//         {
//             digitalWrite(wifiLed, HIGH);
//         }
//         else
//         {
//             digitalWrite(wifiLed, LOW);
//         }

//         wifiPrevious = millis();
//     }
// }

// void sendReadings(int percentage)
// {
//     if (percentage == 0) // do not send more than one time in 10 seconds
//     {
//         if (millis() - previousEmpty < 10000)
//             return;

//         previousEmpty = millis();
//     }

//     if (percentage == 10) // do not send more than one time in 10 seconds
//     {
//         if (millis() - previousFull < 10000)
//             return;

//         previousFull = millis();
//     }

//     if (WiFi.status() == WL_CONNECTED)
//     {
//         HTTPClient http;
//         int httpCode;

//         String LED_State = "";

//         postData = "percentage=" + String(percentage);

//         payload = "";
//         http.begin(client, "http://192.168.8.55/tank/web/php_codes/newReading.php");
//         http.addHeader("Content-Type", "application/x-www-form-urlencoded");
//         httpCode = http.POST(postData);
//         payload = http.getString(); // return nothing
//         http.end();
//     }
// }

void control()
{
    // Serial.println(digitalRead(D6));
    led.moniterBtn();
}