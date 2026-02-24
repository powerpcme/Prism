#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class RateShifter
{
public:
    RateShifter();

    void prepare(double sampleRate);
    void reset();

    // Set pitch shift amount (0 = -1 octave, 0.5 = normal, 1 = +1 octave)
    void setPitchShift(float shift);

    // Process with linear interpolation-based resampling
    float process(float input);

private:
    double sampleRate = 44100.0;
    float pitchRatio = 1.0f;

    // Simple delay line for interpolation
    static constexpr int BUFFER_SIZE = 4096;
    std::array<float, BUFFER_SIZE> buffer;
    double writePos = 0.0;
    double readPos = 0.0;
};
