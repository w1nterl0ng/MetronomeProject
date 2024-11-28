#pragma once

#include <Arduino.h>

class Metronome
{
public:
    Metronome();
    void begin();

    // Core metronome control
    void start();
    void stop();
    bool isRunning() const { return running; }

    // Tempo management
    void setTempo(int newTempo);
    int getTempo() const { return tempo; }

    // Tap tempo functionality
    void tap();
    bool isInTapMode() const { return tapMode; }

    // Main update function - call this in loop()
    void update(bool displayActive = true);

    // Configuration
    void setLiveGigMode(bool enabled) { liveGigMode = enabled; }

private:
    bool running;
    bool tapMode;
    bool liveGigMode;
    int tempo;
    unsigned long lastBeat;
    unsigned long lastTapTime;

    void generateBeat();
    void updateTapTempo();
};