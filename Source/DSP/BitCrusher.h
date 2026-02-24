#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class BitCrusher
{
public:
    BitCrusher() = default;

    void prepare(double sampleRate);
    void reset();

    // Set sample rate reduction (0 = no reduction, 1 = maximum reduction)
    void setSampleRateReduction(float srr);

    // Set bit depth (0 = 1 bit, 1 = full resolution)
    void setBitDepth(float bitrate);

    float process(float input);

private:
    double hostSampleRate = 44100.0;

    // Sample rate reduction
    float srrAmount = 0.0f;
    float sampleHoldCounter = 0.0f;
    float sampleHoldIncrement = 1.0f;
    float heldSample = 0.0f;

    // Bit crushing
    float quantizationLevels = 65536.0f;
};
