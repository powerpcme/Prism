#include "RateShifter.h"
#include <cmath>

RateShifter::RateShifter()
{
    reset();
}

void RateShifter::prepare(double sr)
{
    sampleRate = sr;
    reset();
}

void RateShifter::reset()
{
    buffer.fill(0.0f);
    writePos = 0.0;
    readPos = 0.0;
}

void RateShifter::setPitchShift(float shift)
{
    // Map 0-1 to pitch ratio
    // 0 = 0.5x (down one octave)
    // 0.5 = 1.0x (normal)
    // 1 = 2.0x (up one octave)
    pitchRatio = std::pow(2.0f, (shift - 0.5f) * 2.0f);
}

float RateShifter::process(float input)
{
    // Write input to buffer
    auto writeIdx = static_cast<size_t>(static_cast<int>(writePos) % BUFFER_SIZE);
    buffer[writeIdx] = input;
    writePos += 1.0;

    if (writePos >= BUFFER_SIZE)
        writePos -= BUFFER_SIZE;

    // Read with interpolation
    auto idx0 = static_cast<size_t>(static_cast<int>(readPos) % BUFFER_SIZE);
    auto idx1 = static_cast<size_t>((static_cast<int>(readPos) + 1) % BUFFER_SIZE);
    float frac = static_cast<float>(readPos - std::floor(readPos));

    float sample0 = buffer[idx0];
    float sample1 = buffer[idx1];
    float output = sample0 + frac * (sample1 - sample0);

    // Advance read position based on pitch ratio
    readPos += pitchRatio;

    if (readPos >= BUFFER_SIZE)
        readPos -= BUFFER_SIZE;
    if (readPos < 0)
        readPos += BUFFER_SIZE;

    // Keep read position from getting too far from write position
    double distance = writePos - readPos;
    if (distance < 0)
        distance += BUFFER_SIZE;

    if (distance > BUFFER_SIZE / 2)
        readPos = writePos - BUFFER_SIZE / 4;
    else if (distance < BUFFER_SIZE / 4)
        readPos = writePos - BUFFER_SIZE / 2;

    return output;
}
