#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class EnvelopeShaper
{
public:
    EnvelopeShaper() = default;

    void prepare(double sampleRate);
    void reset();

    // Trigger envelope from start
    void trigger();

    // Set envelope amount (0 = no envelope, 1 = full envelope)
    void setEnvelopeAmount(float amount);

    // Set envelope rate (-1 = fastest attack, 0 = medium, 1 = slowest decay)
    void setEnvelopeRate(float rate);

    // Set duration in samples
    void setDuration(int samples);

    float process();

    bool isActive() const { return active; }
    float getCurrentValue() const { return currentValue; }

private:
    double sampleRate = 44100.0;

    bool active = false;
    float currentValue = 0.0f;
    float envelopeAmount = 1.0f;
    float envelopeRate = 0.0f;

    int currentSample = 0;
    int totalSamples = 44100; // 1 second default

    // Curve shaping
    float curveExponent = 1.0f;
};
