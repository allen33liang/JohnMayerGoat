#pragma once

#include "KnobLookAndFeel.h"
#include "dsp/AudioEngine.h"

class IOPageComponent : public juce::Component,
                        private juce::Timer
{
public:
    IOPageComponent(AudioEngine& engineToUse, juce::AudioDeviceManager& devicesToUse);
    ~IOPageComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void visibilityChanged() override;
    void sync();

    static void openAudioSettings(juce::AudioDeviceManager& devices);

private:
    void timerCallback() override;

    AudioEngine& engine;
    juce::AudioDeviceManager& devices;
    KnobLookAndFeel knobsLaf;

    juce::Slider inputGain, outputGain;
    juce::Label inputLabel { {}, "INPUT GAIN" };
    juce::Label outputLabel { {}, "OUTPUT VOLUME" };
    juce::Label inputValue, outputValue;
    juce::TextButton audioButton { "Audio Settings" };

    float inMeter = 0.0f;
    float outMeter = 0.0f;
    juce::Rectangle<int> inMeterBounds, outMeterBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IOPageComponent)
};
