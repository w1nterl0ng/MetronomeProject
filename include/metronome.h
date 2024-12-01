#pragma once

#include <Arduino.h>

class Metronome
{
public:
    Metronome();
    void begin();
    void update(bool displayActive);
    void start();
    void stop();
    void tap();
    void setTempo(int newTempo);
    int getTempo() const { return tempo; }
    bool isRunning() const { return running; }
    bool isInTapMode() const { return tapMode; }
    void setLiveGigMode(bool enabled) { liveGigMode = enabled; }

private:
    bool running;
    bool tapMode;
    bool liveGigMode;
    int tempo;
    unsigned long lastBeat;
    unsigned long lastTapTime;

    void generateBeat(bool displayActive);
};