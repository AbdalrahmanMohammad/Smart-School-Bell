#ifndef Bulb_h
#define Bulb_h

#include <Arduino.h>
#include <Toggelable.h>

class Bulb : public Togglable
{
private:
    byte pin;
    byte buttonPin; // it is optional to use
    String state;
    boolean hasbutton;
    boolean offState = LOW;
    boolean onState = HIGH;
    
    // EMI filtering variables - time-based hold detection
    unsigned long buttonPressStartTime = 0;  // When button first went LOW
    bool buttonPressDetected = false;  // Flag to track if we're timing a press
    bool buttonActionExecuted = false;  // Prevents multiple toggles while holding
    static const unsigned long BUTTON_HOLD_TIME_MS = 1000;  // 1 second hold for maximum EMI protection

public:
    Bulb(byte pin)
    {
        hasbutton = false;
        this->pin = pin;
        state = "off";
        // previous = 0UL;
        // duration = 0UL;
        // startTime = 0UL;
        btncurstate = HIGH;  //
        btnprevstate = HIGH; //
        buttonPin = -1;
    }

    Bulb(byte pin, byte buttonPin) : Bulb(pin)
    {
        setButton(buttonPin);
    }

    virtual void init()
    {
        if (hasButton())
        {
            pinMode(buttonPin, INPUT_PULLUP);
        }
        pinMode(pin, OUTPUT);
        off();
    }

    virtual void on() override
    {
        digitalWrite(pin, onState);
        state = "on";
    }
    virtual void off() override
    {
        digitalWrite(pin, offState);
        state = "off";
    }
    virtual void toggle() override
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

    virtual bool isOn()
    {
        return (state == "on");
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
                    persistState(); // Save state only on physical button press
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

    // Helper method for state persistence
    void persistState()
    {
        StaticJsonDocument<128> cfg; // Reduced size for better performance
        File f = LittleFS.open("/config.json", "r");
        if (f)
        {
            String c = f.readString();
            f.close();
            DeserializationError e = deserializeJson(cfg, c);
            if (e) cfg.clear();
        }
        cfg["bulbOn"] = isOn();
        File wf = LittleFS.open("/config.json", "w");
        if (wf)
        {
            serializeJson(cfg, wf);
            wf.close();
        }
    }
};

#endif