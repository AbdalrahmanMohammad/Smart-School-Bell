#ifndef LED_h
#define LED_h

#include <Arduino.h>
#include <Toggelable.h>

class LED : public Togglable
{
private:
    byte pin;
    byte buttonPin; // it is optional to use
    boolean state;
    boolean hasbutton;

public:
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
        digitalWrite(pin, HIGH);
        state = HIGH;
    }
    virtual void off() override
    {
        digitalWrite(pin, LOW);
        state = LOW;
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

        if ((btncurstate == LOW) && (btnprevstate == HIGH) && (millis() - previous > 500)) // button pressed and debounce
        {
            previous = millis(); // for debounce
            toggle();
            btnprevstate = btncurstate;
        }
        btnprevstate = btncurstate;
    }
};

#endif