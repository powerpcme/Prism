#include "AMModulator.h"
#include <cmath>

void AMModulator::prepare(double sr)
{
    sampleRate = sr;
    reset();
    setFrequency(frequency);
}

void AMModulator::reset()
{
    phase = 0.0;
}

void AMModulator::setDepth(float d)
{
    depth = d;
}

void AMModulator::setFrequency(float freqHz)
{
    frequency = freqHz;
    phaseIncrement = (2.0 * juce::MathConstants<double>::pi * frequency) / sampleRate;
}

float AMModulator::process(float input)
{
    if (depth < 0.001f)
        return input;

    // Generate sine wave modulator
    float modulator = static_cast<float>(std::sin(phase));

    // Advance phase
    phase += phaseIncrement;
    if (phase >= 2.0 * juce::MathConstants<double>::pi)
        phase -= 2.0 * juce::MathConstants<double>::pi;

    // Ring modulation: (1 - depth) + depth * modulator
    // At depth 0: output = input * 1
    // At depth 1: output = input * modulator (pure ring mod)
    float modulationAmount = (1.0f - depth) + depth * (modulator * 0.5f + 0.5f);

    return input * modulationAmount;
}
