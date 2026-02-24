#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class Overdrive
{
public:
    Overdrive() = default;

    void prepare(double sampleRate);
    void reset();

    // Set drive amount (0 = clean, 1 = max distortion)
    void setDrive(float drive);

    float process(float input);

private:
    double sampleRate = 44100.0;
    float driveAmount = 0.0f;
    float preGain = 1.0f;
    float postGain = 1.0f;

    // DC blocker
    float dcBlockerX1 = 0.0f;
    float dcBlockerY1 = 0.0f;
    static constexpr float DC_BLOCKER_COEFF = 0.995f;
};
