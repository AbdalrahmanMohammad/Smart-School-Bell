#ifndef LED_h
#define LED_h

#include <Arduino.h>
#include <Toggelable.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

class LED : public Togglable
{
private:
    byte pin;
    byte buttonPin; // it is optional to use
    boolean state;
    boolean hasbutton;
    
    // EMI filtering variables - time-based hold detection
    unsigned long buttonPressStartTime = 0;  // When button first went LOW
    bool buttonPressDetected = false;  // Flag to track if we're timing a press
    bool buttonActionExecuted = false;  // Prevents multiple toggles while holding
    static const unsigned long BUTTON_HOLD_TIME_MS = 1000;  // 1 second hold for maximum EMI protection

public:
    boolean checkNow = false;// just for applying the schedule immediately when the led is turned on
    boolean gotOff=false;
    LED(byte pin)
    {
        hasbutton = false;
        this->pin = pin;
        state = LOW;
        // previous = 0UL;
        // duration = 0UL;
        // startTime = 0UL;
        btncurstate = HIGH;  //
        btnprevstate = HIGH; //
        buttonPin = -1;
    }

    LED(byte pin, byte buttonPin) : LED(pin)
    { // i called the first constructor
        setButton(buttonPin);
    }

    virtual void init()
    {
        if (hasButton())
        {
            pinMode(buttonPin, INPUT_PULLUP);
        }
        pinMode(pin, OUTPUT);
    }
    virtual void init(byte defaultState)
    {
        init();
        if (defaultState == HIGH)
        {
            on();
        }
        else
        {
            off();
        }
    }

    virtual void on() override
    {
        checkNow = true;
        digitalWrite(pin, HIGH);
        state = HIGH;
        // Persist LED state
        StaticJsonDocument<256> cfg;
        File f = LittleFS.open("/config.json", "r");
        if (f)
        {
            String c = f.readString();
            f.close();
            DeserializationError e = deserializeJson(cfg, c);
            if (e)
                cfg.clear();
        }
        cfg["ledOn"] = true;
        File wf = LittleFS.open("/config.json", "w");
        if (wf)
        {
            serializeJson(cfg, wf);
            wf.close();
        }
    }
    virtual void off() override
    {
        digitalWrite(pin, LOW);
        state = LOW;
        gotOff=true;
        // Persist LED state
        StaticJsonDocument<256> cfg;
        File f = LittleFS.open("/config.json", "r");
        if (f)
        {
            String c = f.readString();
            f.close();
            DeserializationError e = deserializeJson(cfg, c);
            if (e)
                cfg.clear();
        }
        cfg["ledOn"] = false;
        File wf = LittleFS.open("/config.json", "w");
        if (wf)
        {
            serializeJson(cfg, wf);
            wf.close();
        }
    }

    virtual bool isOn()
    {
        return (state == HIGH);
    }

    virtual void toggle() override // you can just digialWrite(pin,!digitalRead(pin)); but this is better
    {
        if (isOn())
        {
            off();
        }
        else
        {
            on();
        }
    }

    virtual void setButton(int i)
    {
        hasbutton = true;
        buttonPin = i;
    }

    virtual byte btn()
    {
        return buttonPin;
    }

    virtual bool hasButton()
    {
        return hasbutton;
    }

    virtual bool btnstate()
    {
        return digitalRead(buttonPin);
    }

    virtual void moniterBtn()
    {
        if (!hasButton())
            return;

        btncurstate = btnstate();

        // EMI-resistant time-based hold filtering
        if (btncurstate == LOW)
        {
            if (!buttonPressDetected && !buttonActionExecuted)
            {
                // Button just went LOW - start timing
                buttonPressStartTime = millis();
                buttonPressDetected = true;
            }
            else if (buttonPressDetected && !buttonActionExecuted)
            {
                // Button is still LOW - check if held long enough
                if ((millis() - buttonPressStartTime >= BUTTON_HOLD_TIME_MS) && 
                    (millis() - previous > 500))  // Overall debounce between toggles
                {
                    // Button held for required time - it's a real press!
                    previous = millis();
                    toggle();
                    buttonActionExecuted = true;  // Prevent re-triggering until released
                }
            }
            // If buttonActionExecuted is true, do nothing - wait for release
        }
        else
        {
            // Button is HIGH (released) - reset everything for next press
            buttonPressDetected = false;
            buttonActionExecuted = false;
        }
        
        btnprevstate = btncurstate;
    }

    virtual void loop()
    {
        moniterBtn();
    }
};

#endif