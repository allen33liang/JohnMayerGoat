#pragma once

#include <JuceHeader.h>
#include "dsp/AudioEngine.h"

class PresetManager
{
public:
    juce::File getPresetFolder() const;
    juce::StringArray listPresetNames() const;
    bool savePreset(const juce::String& name, const juce::var& json) const;
    juce::var loadPreset(const juce::String& name) const;
};
