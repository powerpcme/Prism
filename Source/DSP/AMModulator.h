#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class AMModulator
{
public:
    AMModulator() = default;

    void prepare(double sampleRate);
    void reset();

    // Set modulation depth (0 = no modulation, 1 = full modulation)
    void setDepth(float depth);

    // Set modulation frequency in Hz
    void setFrequency(float freqHz);

    float process(float input);

private:
    double sampleRate = 44100.0;
    float depth = 0.0f;
    float frequency = 12000.0f; // 12kHz default as per original Max patch
    double phase = 0.0;
    double phaseIncrement = 0.0;
};
