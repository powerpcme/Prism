#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

enum class SyncRate
{
    RATE_1_1 = 0,    // Whole note
    RATE_1_2,        // Half note
    RATE_1_4,        // Quarter note
    RATE_1_8,        // Eighth note
    RATE_1_16,       // Sixteenth note
    RATE_1_32,       // 32nd note
    RATE_1_64,       // 64th note
    RATE_1_8T,       // Eighth triplet
    RATE_1_16T,      // Sixteenth triplet
    NUM_RATES
};

class SyncDivisions
{
public:
    // Get the beat multiplier for a sync rate
    static double getMultiplier(SyncRate rate)
    {
        switch (rate)
        {
            case SyncRate::RATE_1_1:  return 4.0;
            case SyncRate::RATE_1_2:  return 2.0;
            case SyncRate::RATE_1_4:  return 1.0;
            case SyncRate::RATE_1_8:  return 0.5;
            case SyncRate::RATE_1_16: return 0.25;
            case SyncRate::RATE_1_32: return 0.125;
            case SyncRate::RATE_1_64: return 0.0625;
            case SyncRate::RATE_1_8T: return 1.0 / 3.0;
            case SyncRate::RATE_1_16T: return 1.0 / 6.0;
            case SyncRate::NUM_RATES: return 1.0;
        }
        return 1.0;
    }

    // Get the display name for a sync rate
    static juce::String getName(SyncRate rate)
    {
        switch (rate)
        {
            case SyncRate::RATE_1_1:  return "1/1";
            case SyncRate::RATE_1_2:  return "1/2";
            case SyncRate::RATE_1_4:  return "1/4";
            case SyncRate::RATE_1_8:  return "1/8";
            case SyncRate::RATE_1_16: return "1/16";
            case SyncRate::RATE_1_32: return "1/32";
            case SyncRate::RATE_1_64: return "1/64";
            case SyncRate::RATE_1_8T: return "1/8T";
            case SyncRate::RATE_1_16T: return "1/16T";
            case SyncRate::NUM_RATES: return "1/4";
        }
        return "1/4";
    }

    // Get all rate names for combo box
    static juce::StringArray getAllNames()
    {
        return {"1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64", "1/8T", "1/16T"};
    }
};
