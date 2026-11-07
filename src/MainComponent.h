#pragma once

#include <JuceHeader.h>
#include "dsp/AudioEngine.h"
#include "gui/AmpPageComponent.h"
#include "gui/IOPageComponent.h"
#include "gui/KnobLookAndFeel.h"
#include "gui/PedalboardComponent.h"
#include "presets/PresetManager.h"

class MainComponent : public juce::Component,
                      public juce::ChangeListener
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

private:
    enum class Page { Pedals, Amp, IO };

    void refreshPresetList();
    void saveCurrentPreset();
    void loadSelectedPreset();
    void showAudioSettings();
    void syncGlobalControls();
    void showPage(Page page);

    juce::AudioDeviceManager deviceManager;
    AudioEngine engine;
    PresetManager presets;
    KnobLookAndFeel knobsLaf;

    juce::Label title { {}, "JohnMayerGoat" };
    juce::TextButton audioButton { "Audio Settings" };
    juce::TextButton bypassButton { "Master Bypass" };
    juce::TextButton resetButton { "Factory Reset" };
    juce::TextButton saveButton { "Save" };
    juce::Label makePresetLabel { {}, "MAKE PRESET" };
    juce::Label loadPresetLabel { {}, "LOAD PRESET" };
    juce::ComboBox presetBox;
    juce::TextEditor presetName;

    juce::TextButton pedalsTab { "PEDALS" };
    juce::TextButton ampTab { "AMP" };
    juce::TextButton ioTab { "INPUT & OUTPUT" };

    PedalboardComponent pedalboard;
    AmpPageComponent ampPage;
    IOPageComponent ioPage;
    Page currentPage = Page::Pedals;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
