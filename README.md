## ⚙️ Time-Range Logic for Bulb Control

> **Note:**  
> This part of the code **expects that the ON/OFF orientation is the same for all days.**  
> For example, if "ON" comes before "OFF" on one day, the same order should apply consistently across all days.

The following logic determines whether the bulb should be **ON** or **OFF** based on the current time and the configured ON/OFF times, with performance optimization in mind.

```cpp
// Apply time-range logic with performance optimization
if (currentMinutes < firstTime && currentMinutes < secondTime)
{
    // Current time is before both times - apply the state of the second time
    if (cachedOnMinutes < cachedOffMinutes)
    {
        // ON comes first, OFF comes second - so before both means OFF state
        bulb.off();
    }
    else
    {
        // OFF comes first, ON comes second - so before both means ON state
        bulb.on();
    }
}
else if (currentMinutes > firstTime && currentMinutes >= secondTime)
{
    // Current time is after both times - apply the state of the second time
    if (cachedOnMinutes < cachedOffMinutes)
    {
        // ON comes first, OFF comes second - so after both means OFF state
        bulb.off();
    }
    else
    {
        // OFF comes first, ON comes second - so after both means ON state
        bulb.on();
    }
}
else if (currentMinutes >= firstTime && currentMinutes < secondTime)
{
    // Current time is between the two times
    if (cachedOnMinutes < cachedOffMinutes)
    {
        // ON comes first, so we're in the ON period
        bulb.on();
    }
    else
    {
        // OFF comes first, so we're in the OFF period
        bulb.off();
    }
}
