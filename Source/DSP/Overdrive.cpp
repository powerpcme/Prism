#include "Overdrive.h"
#include <cmath>

void Overdrive::prepare(double sr)
{
    sampleRate = sr;
    reset();
}

void Overdrive::reset()
{
    dcBlockerX1 = 0.0f;
    dcBlockerY1 = 0.0f;
}

void Overdrive::setDrive(float drive)
{
    driveAmount = drive;

    // Pre-gain increases with drive, post-gain compensates
    preGain = 1.0f + drive * 20.0f;
    postGain = 1.0f / (1.0f + drive * 2.0f);
}

float Overdrive::process(float input)
{
    if (driveAmount < 0.001f)
        return input;

    // Apply pre-gain
    float x = input * preGain;

    // Tanh waveshaping (soft clipping)
    float shaped = std::tanh(x);

    // Apply post-gain for level compensation
    shaped *= postGain;

    // Mix dry/wet based on drive amount
    float output = input * (1.0f - driveAmount) + shaped * driveAmount;

    // DC blocker to remove any DC offset
    float dcBlocked = output - dcBlockerX1 + DC_BLOCKER_COEFF * dcBlockerY1;
    dcBlockerX1 = output;
    dcBlockerY1 = dcBlocked;

    return dcBlocked;
}
