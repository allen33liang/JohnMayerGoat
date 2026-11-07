#pragma once

#include "KnobLookAndFeel.h"
#include "dsp/AudioEngine.h"
#include "dsp/DigitalPreamp.h"

class AmpPageComponent : public juce::Component
{
public:
    explicit AmpPageComponent(AudioEngine& engineToUse);
    ~AmpPageComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void refresh();

private:
    struct Layout
    {
        juce::Rectangle<int> combo;
        juce::Rectangle<int> head;
        juce::Rectangle<int> face;
        juce::Rectangle<int> knobRow;
        juce::Rectangle<int> switchRow;
        juce::Rectangle<int> cab;
        juce::Rectangle<int> power;
        juce::Rectangle<int> cabPanel;
        juce::Rectangle<int> modelBar;
        juce::Rectangle<float> jewel;
        juce::Rectangle<float> handle;
    };

    Layout computeLayout() const;
    void paintHead(juce::Graphics& g, const Layout& layout) const;
    void paintCabinet(juce::Graphics& g, juce::Rectangle<float> cab) const;
    void paintGrill(juce::Graphics& g, juce::Rectangle<float> cloth) const;
    void paintToggle(juce::Graphics& g, juce::Rectangle<float> area, bool on) const;
    void updateCabEnabled();
    void showValue(juce::Slider& slider);
    void refreshIrList();
    void chooseIrFolder();
    void rebuildAmpControls();
    DigitalPreamp* preamp() const;
    KnobStyle knobStyleForModel(int model) const;
    juce::Colour labelColourForModel(int model) const;

    AudioEngine& engine;
    KnobLookAndFeel knobsLaf;
    juce::ComboBox modelBox;
    juce::TextButton powerButton;
    std::vector<std::unique_ptr<juce::Slider>> sliders;
    std::vector<std::unique_ptr<juce::Label>> sliderLabels;
    std::vector<std::unique_ptr<juce::TextButton>> switches;
    juce::Label valueReadout;
    juce::Slider* liveSlider = nullptr;

    juce::TextButton cabEnable { "CAB SIM OFF" };
    juce::Slider cabBody, cabAir, cabLevel;
    juce::Label cabBodyLabel { {}, "BODY" };
    juce::Label cabAirLabel { {}, "AIR" };
    juce::Label cabLevelLabel { {}, "LEVEL" };
    juce::ComboBox irBox;
    juce::TextButton irFolderButton { "Choose IR folder..." };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AmpPageComponent)
};
