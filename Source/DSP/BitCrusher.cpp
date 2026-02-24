#include "BitCrusher.h"
#include <cmath>

void BitCrusher::prepare(double sampleRate)
{
    hostSampleRate = sampleRate;
    reset();
}

void BitCrusher::reset()
{
    sampleHoldCounter = 0.0f;
    heldSample = 0.0f;
}

void BitCrusher::setSampleRateReduction(float srr)
{
    srrAmount = srr;

    // Map 0-1 to sample rate reduction factor
    // 0 = no reduction (increment = 1)
    // 1 = reduce to ~500Hz
    if (srr < 0.001f)
    {
        sampleHoldIncrement = 1.0f;
    }
    else
    {
        // Target sample rate: 44100 down to 500 Hz
        float targetRate = hostSampleRate * (1.0f - srr * 0.99f);
        targetRate = juce::jmax(500.0f, static_cast<float>(targetRate));
        sampleHoldIncrement = targetRate / static_cast<float>(hostSampleRate);
    }
}

void BitCrusher::setBitDepth(float bitrate)
{
    // Map 0-1 to bit depth
    // 0 = 1 bit (2 levels)
    // 1 = 16 bit (65536 levels)
    float bits = 1.0f + bitrate * 15.0f;
    quantizationLevels = std::pow(2.0f, bits);
}

float BitCrusher::process(float input)
{
    // Sample rate reduction (sample and hold)
    sampleHoldCounter += sampleHoldIncrement;

    if (sampleHoldCounter >= 1.0f)
    {
        sampleHoldCounter -= 1.0f;
        heldSample = input;
    }

    float output = heldSample;

    // Bit crushing (quantization)
    if (quantizationLevels < 65535.0f)
    {
        // Quantize to specified number of levels
        output = std::round(output * quantizationLevels) / quantizationLevels;
    }

    return output;
}
