#pragma once

#include <JuceHeader.h>

enum class KnobStyle
{
    Generic = 0,
    ChickenHead,
    SilverCap,
    BluePointer,
    WhitePointer,
    Aqua,
    AmpVintage,
    Bakelite,
    Knurled,
    AmpSkirted,
    AmpChrome,
    AmpBlack,
    AmpToggle
};

enum class FootswitchStyle
{
    None = 0,
    Metal,
    Rubber
};

class KnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    KnobLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    static void applyKnobStyle(juce::Slider& slider, KnobStyle style);
    static KnobStyle styleFrom(const juce::Slider& slider);
    static void applyFootswitchStyle(juce::Button& button, FootswitchStyle style);
    static void applyChrome(juce::Button& button, const juce::String& chromeId);

private:
    void drawChickenHead(juce::Graphics& g, juce::Point<float> centre, float radius,
                         float angle, juce::Colour body, juce::Colour pointer) const;
    void drawCappedKnob(juce::Graphics& g, juce::Point<float> centre, float radius,
                        float angle, juce::Colour skirt, juce::Colour cap, juce::Colour pointer) const;
    void drawDotPointer(juce::Graphics& g, juce::Point<float> centre, float radius,
                        float angle, juce::Colour body, juce::Colour pointer, bool triangle) const;
    void drawKnurled(juce::Graphics& g, juce::Point<float> centre, float radius, float angle) const;
    void drawSkirted(juce::Graphics& g, juce::Point<float> centre, float radius, float angle,
                     float start, float end) const;
    void drawAmpToggle(juce::Graphics& g, juce::Rectangle<float> bounds, float sliderPos) const;
    void drawTicks(juce::Graphics& g, juce::Point<float> centre, float radius,
                   float start, float end) const;
};
