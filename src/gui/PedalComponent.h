#pragma once

#include "KnobLookAndFeel.h"
#include "dsp/Effect.h"
#include "dsp/EffectChain.h"

class PedalComponent : public juce::Component,
                       public juce::DragAndDropTarget
{
public:
    PedalComponent(Effect& effectToControl, EffectChain& chainToQuery, std::function<void()> onChanged);
    ~PedalComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    bool isInterestedInDragSource(const SourceDetails& details) override;
    void itemDropped(const SourceDetails& details) override;
    void itemDragMove(const SourceDetails& details) override;

    Effect& getEffect() { return effect; }
    juce::String getEffectId() const { return effect.getId(); }
    void refreshFromEffect();
    int preferredWidth(int availableHeight) const;

private:
    struct KnobCtrl
    {
        juce::String id;
        std::unique_ptr<juce::Slider> slider;
        std::unique_ptr<juce::Label> label;
    };

    juce::Rectangle<int> chassisBounds() const;
    juce::Slider* sliderFor(const juce::String& paramId) const;
    juce::Label* labelFor(const juce::String& paramId) const;
    KnobStyle knobStyleForEffect() const;
    FootswitchStyle footswitchStyleForEffect() const;
    juce::Colour labelColour() const;
    void layoutKnobs(juce::Rectangle<int> chassis);
    void placeKnob(const juce::String& paramId, juce::Rectangle<int> knobArea, juce::Rectangle<int> labelArea);
    void placeLed(juce::Rectangle<int> bounds);
    void paintLed(juce::Graphics& g, juce::Rectangle<float> led, bool on) const;

    class LedDot : public juce::Component
    {
    public:
        void setOn(bool shouldBeOn) { lit = shouldBeOn; repaint(); }
        void paint(juce::Graphics& g) override;

    private:
        bool lit = false;
    };

    class TypeToggle : public juce::Button
    {
    public:
        TypeToggle() : juce::Button("type") {}
        void setIndex(int i) { position = juce::jlimit(0, 2, i); repaint(); }
        int getIndex() const { return position; }

        void paintButton(juce::Graphics& g, bool highlighted, bool) override
        {
            auto r = getLocalBounds().toFloat().reduced(1.0f);
            auto labels = r.removeFromRight(juce::jmax(28.0f, r.getWidth() * 0.48f));
            auto boot = r.reduced(4.0f, 6.0f);

            g.setColour(juce::Colours::black.withAlpha(0.35f));
            g.fillRoundedRectangle(boot.translated(0.8f, 1.6f), 6.0f);
            juce::ColourGradient metal(juce::Colour(0xff4a4640), boot.getTopLeft(),
                                       juce::Colour(0xff1a1814), boot.getBottomRight(), false);
            g.setGradientFill(metal);
            g.fillRoundedRectangle(boot, 6.0f);
            g.setColour(juce::Colour(0xff8a8478).withAlpha(0.6f));
            g.drawRoundedRectangle(boot, 6.0f, 1.1f);

            const float ys[3] = { boot.getY() + 10.0f, boot.getCentreY(), boot.getBottom() - 10.0f };
            for (float y : ys)
            {
                g.setColour(juce::Colours::black.withAlpha(0.5f));
                g.fillEllipse(boot.getCentreX() - 3.2f, y - 1.5f, 6.4f, 3.0f);
            }

            const float ly = ys[juce::jlimit(0, 2, position)];
            auto bat = juce::Rectangle<float>(11.0f, 24.0f).withCentre({ boot.getCentreX(), ly });
            if (highlighted)
                bat = bat.expanded(0.7f);
            g.setColour(juce::Colours::black.withAlpha(0.4f));
            g.fillRoundedRectangle(bat.translated(0.7f, 1.4f), 5.0f);
            juce::ColourGradient chrome(juce::Colour(0xfff6f2ea), bat.getTopLeft(),
                                        juce::Colour(0xff8a8478), bat.getBottomRight(), false);
            g.setGradientFill(chrome);
            g.fillRoundedRectangle(bat, 5.0f);
            g.setColour(juce::Colours::white.withAlpha(0.5f));
            g.drawLine(bat.getX() + 2.4f, bat.getY() + 3.5f, bat.getRight() - 2.4f, bat.getY() + 3.5f, 1.2f);

            const char* names[3] = { "ROOM", "PLATE", "SPRING" };
            g.setFont(juce::Font(juce::FontOptions("Avenir Next Condensed", 8.0f, juce::Font::bold)));
            for (int i = 0; i < 3; ++i)
            {
                g.setColour(i == position ? juce::Colour(0xff1a120c) : juce::Colour(0xff5a4a38).withAlpha(0.7f));
                g.drawText(names[i], juce::Rectangle<float>(labels.getX(), ys[i] - 7.0f,
                                                            labels.getWidth(), 14.0f).toNearestInt(),
                           juce::Justification::centredLeft);
            }
        }

    private:
        int position = 0;
    };

    void paintTubeScreamer(juce::Graphics& g, juce::Rectangle<float> b);
    void paintBluesBreaker(juce::Graphics& g, juce::Rectangle<float> b);
    void paintRat(juce::Graphics& g, juce::Rectangle<float> b);
    void paintShredmaster(juce::Graphics& g, juce::Rectangle<float> b);
    void paintChorus(juce::Graphics& g, juce::Rectangle<float> b);
    void paintEchoplex(juce::Graphics& g, juce::Rectangle<float> b);
    void paintAquaPuss(juce::Graphics& g, juce::Rectangle<float> b);
    void paintGolden(juce::Graphics& g, juce::Rectangle<float> b);

    Effect& effect;
    EffectChain& chain;
    std::function<void()> onChanged;
    KnobLookAndFeel knobsLaf;
    juce::TextButton bypassButton;
    juce::Label valueReadout;
    LedDot led;
    TypeToggle typeToggle;
    std::vector<KnobCtrl> knobs;
    juce::Slider* liveSlider = nullptr;
};
