#ifndef Bulb_h
#define Bulb_h

#include <Arduino.h>
#include <Toggelable.h>

class Bulb : public Togglable
{
private:
    byte pin;
    byte buttonPin; // it is optional to use
    boolean hasbutton;
    boolean offState = LOW;
    boolean onState = HIGH;

public:
    Bulb(byte pin)
    {
        hasbutton = false;
        this->pin = pin;
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
        // setDuration(3000UL); // default duration 5 seconds
    }

    virtual void on() override
    {

            digitalWrite(pin, onState);
    }
    virtual void off() override
    {
        digitalWrite(pin, offState);
    }
    virtual void toggle() override
    {
        if (isOn()) {
            off();
        } else {
            on();
        }
    }

    virtual bool isOn()
    {
        return digitalRead(pin) == onState;
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

        if ((btncurstate == LOW) && (btnprevstate == HIGH) && (millis() - previous > 500)) // button pressed and debounce
        {
            previous = millis(); // for debounce
            on();
            btnprevstate = btncurstate;
        }
        btnprevstate = btncurstate;
    }

    virtual void loop()
    {
        moniterBtn();
    }
};

#endif