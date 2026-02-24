#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class VoiceElement
{
public:
    // Get key label for this voice (color name)
    static juce::String getKeyLabel(int index);

    // Get the colour for a voice index
    static juce::Colour getVoiceColour(int index);
};
