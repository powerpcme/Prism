#include "EnvelopeShaper.h"
#include <cmath>

void EnvelopeShaper::prepare(double sr)
{
    sampleRate = sr;
    reset();
}

void EnvelopeShaper::reset()
{
    active = false;
    currentValue = 0.0f;
    currentSample = 0;
}

void EnvelopeShaper::trigger()
{
    active = true;
    currentSample = 0;
    currentValue = 1.0f;
}

void EnvelopeShaper::setEnvelopeAmount(float amount)
{
    envelopeAmount = amount;
}

void EnvelopeShaper::setEnvelopeRate(float rate)
{
    envelopeRate = rate;

    // Map rate to curve exponent
    // Negative rate = convex curve (fast attack, slow decay)
    // Zero = linear
    // Positive rate = concave curve (slow attack, fast decay)
    if (rate < 0.0f)
    {
        // Convex (fast start)
        curveExponent = 1.0f + (-rate) * 3.0f;
    }
    else
    {
        // Concave (slow start)
        curveExponent = 1.0f / (1.0f + rate * 3.0f);
    }
}

void EnvelopeShaper::setDuration(int samples)
{
    totalSamples = juce::jmax(1, samples);
}

float EnvelopeShaper::process()
{
    if (!active)
        return envelopeAmount; // Return envelope amount as baseline when not active

    if (currentSample >= totalSamples)
    {
        active = false;
        currentValue = 0.0f;
        return 0.0f;
    }

    // Calculate linear position (1 to 0)
    float linearPos = 1.0f - static_cast<float>(currentSample) / static_cast<float>(totalSamples);

    // Apply curve shaping (parabolic/exponential based on rate)
    currentValue = std::pow(linearPos, curveExponent);

    // Mix with envelope amount
    // At amount 0: always 1.0 (no envelope effect)
    // At amount 1: full envelope
    float output = (1.0f - envelopeAmount) + envelopeAmount * currentValue;

    currentSample++;

    return output;
}
