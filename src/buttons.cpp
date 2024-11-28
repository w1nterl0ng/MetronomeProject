#include "buttons.h"
#include "config.h"

Buttons::Buttons() : leftButton(LEFT_SWITCH_PIN),
                     rightButton(RIGHT_SWITCH_PIN),
                     leftPressed(false),
                     rightPressed(false),
                     leftLongPressTriggered(false),
                     rightLongPressTriggered(false)
{
}

void Buttons::begin()
{
    pinMode(leftButton.pin, INPUT_PULLUP);
    pinMode(rightButton.pin, INPUT_PULLUP);
}

bool Buttons::update()
{
    bool stateChanged = false;

    if (handleButton(leftButton, leftPressed, leftLongPressTriggered))
    {
        stateChanged = true;
    }
    if (handleButton(rightButton, rightPressed, rightLongPressTriggered))
    {
        stateChanged = true;
    }

    return stateChanged;
}

bool Buttons::handleButton(Button &button, bool &pressedState, bool &longPressTriggered)
{
    bool stateChanged = false;
    int reading = digitalRead(button.pin);
    unsigned long currentTime = millis();

    if (reading != button.lastState)
    {
        Serial.printf("Pin %d state changed to: %d\n", button.pin, reading);
        button.lastDebounceTime = currentTime;
    }

    if ((currentTime - button.lastDebounceTime) > DEBOUNCE_TIME)
    {
        if (reading != button.currentState)
        {
            button.currentState = reading;

            if (button.currentState == LOW)
            { // Button pressed
                Serial.printf("Button on pin %d pressed\n", button.pin);
                button.pressStartTime = currentTime;
                button.isLongPress = false;
                longPressTriggered = false;
            }
            else
            { // Button released
                if (!button.isLongPress &&
                    (currentTime - button.pressStartTime < HOLD_THRESHOLD))
                {
                    pressedState = true;
                    stateChanged = true;
                    Serial.printf("Short press detected on pin %d\n", button.pin);
                }
            }
        }
        else if (button.currentState == LOW && !button.isLongPress &&
                 currentTime - button.pressStartTime >= HOLD_THRESHOLD)
        {
            button.isLongPress = true;
            longPressTriggered = true;
            stateChanged = true;
            Serial.printf("Long press detected on pin %d\n", button.pin);
        }
    }

    button.lastState = reading;
    return stateChanged;
}

void Buttons::clearButtonStates()
{
    leftPressed = false;
    rightPressed = false;
    leftLongPressTriggered = false;
    rightLongPressTriggered = false;
}