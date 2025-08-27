#ifndef Bell_h
#define Bell_h

#include <Arduino.h>
#include <Toggelable.h>

class Bell : public Togglable
{
private:
    byte pin;
    byte buttonPin; // it is optional to use
    boolean hasbutton;
    boolean offState = LOW;
    boolean onState = HIGH;

public:
    Bell(byte pin)
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

    Bell(byte pin, byte buttonPin) : Bell(pin)
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
        setDuration(3000UL); // default duration 5 seconds
    }

    virtual void on() override
    {
        if (getDuration() > 0UL)
        {
            digitalWrite(pin, onState);
            setStartTime(millis());
        }
    }
    virtual void off() override
    {
        digitalWrite(pin, offState);
    }
    virtual void toggle() override // you can just digialWrite(pin,!digitalRead(pin)); but this is better
    {
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

    virtual void turnOffAfterDuration()
    {
        if (getDuration() > 0UL && isOn() && (millis() - getStartTime()) > getDuration())
        {
            off();
        }
    }

    virtual void loop()
    {
        moniterBtn();
        turnOffAfterDuration();
    }
};

#endif