#include "metronome.h"
#include "config.h"

Metronome::Metronome() : running(false),
                         tapMode(false),
                         liveGigMode(false),
                         tempo(120),
                         lastBeat(0),
                         lastTapTime(0)
{
}

void Metronome::begin()
{
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
}

void Metronome::start()
{
    running = true;
    tapMode = false;
}

void Metronome::stop()
{
    running = false;
    tapMode = false;
    digitalWrite(LED_PIN, LOW);
}

void Metronome::setTempo(int newTempo)
{
    tempo = constrain(newTempo, 40, 240);
}

void Metronome::tap()
{
    unsigned long currentTime = millis();

    if (!tapMode)
    {
        tapMode = true;
        lastTapTime = currentTime;
        return;
    }

    unsigned long tapInterval = currentTime - lastTapTime;
    if (tapInterval < TAP_TIMEOUT)
    {
        int newTempo = 60000 / tapInterval;
        if (newTempo >= 40 && newTempo <= 240)
        { // Validate tempo range
            tempo = newTempo;
            Serial.printf("Tap tempo: %d BPM\n", tempo); // Debug output
        }
    }
    lastTapTime = currentTime;
}

void Metronome::update(bool displayActive)
{
    unsigned long currentTime = millis();

    if (tapMode && (currentTime - lastTapTime > TAP_TIMEOUT))
    {
        tapMode = false;
        running = true;
    }

    if (!running || (liveGigMode && !displayActive))
    {
        digitalWrite(LED_PIN, LOW);
        return;
    }

    generateBeat(displayActive);
}

void Metronome::generateBeat(bool displayActive)
{
    if (!displayActive && liveGigMode)
    {
        digitalWrite(LED_PIN, LOW);
        return;
    }

    unsigned long currentTime = millis();
    unsigned long interval = 60000 / tempo;

    if (currentTime - lastBeat >= interval)
    {
        digitalWrite(LED_PIN, HIGH);
        delayMicroseconds(50000);
        digitalWrite(LED_PIN, LOW);
        lastBeat = currentTime;
    }
}