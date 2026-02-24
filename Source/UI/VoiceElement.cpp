#include "VoiceElement.h"

juce::String VoiceElement::getKeyLabel(int index)
{
    static const char* labels[] = {"Red", "Orange", "Yellow", "Green", "Cyan",
                                   "Blue", "Indigo", "Violet", "Pink", "White"};
    if (index >= 0 && index < 10)
        return labels[index];
    return juce::String(index);
}

juce::Colour VoiceElement::getVoiceColour(int index)
{
    static const juce::Colour colours[] = {
        juce::Colour(0xffff3333),  // Red
        juce::Colour(0xffff8833),  // Orange
        juce::Colour(0xffffdd33),  // Yellow
        juce::Colour(0xff33ff66),  // Green
        juce::Colour(0xff33ddff),  // Cyan
        juce::Colour(0xff3366ff),  // Blue
        juce::Colour(0xff6633ff),  // Indigo
        juce::Colour(0xffaa33ff),  // Violet
        juce::Colour(0xffff66aa),  // Pink
        juce::Colour(0xffeeeeee),  // White
    };
    if (index >= 0 && index < 10)
        return colours[index];
    return juce::Colours::grey;
}
