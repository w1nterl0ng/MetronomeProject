#pragma once

#include <Arduino.h>
#include "types.h"

class Buttons
{
public:
    Buttons();
    void begin();

    // Main update function, returns true if any button state changed
    bool update();

    // Button state queries
    bool wasLeftButtonPressed() const { return leftPressed && !leftButton.isLongPress; }
    bool wasRightButtonPressed() const { return rightPressed && !rightButton.isLongPress; }
    bool isLeftLongPress() const { return leftButton.isLongPress && leftLongPressTriggered; }
    bool isRightLongPress() const { return rightButton.isLongPress && rightLongPressTriggered; }

    // Reset states after handling
    void clearButtonStates();

private:
    Button leftButton;
    Button rightButton;

    bool leftPressed;
    bool rightPressed;
    bool leftLongPressTriggered;
    bool rightLongPressTriggered;

    bool handleButton(Button &button, bool &pressedState, bool &longPressTriggered);
};