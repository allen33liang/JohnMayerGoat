#pragma once

#include "PedalComponent.h"
#include "dsp/AudioEngine.h"

class PedalboardComponent : public juce::Component,
                            public juce::DragAndDropContainer,
                            public juce::DragAndDropTarget
{
public:
    explicit PedalboardComponent(AudioEngine& engineToUse);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void rebuild();

    bool isInterestedInDragSource(const SourceDetails& details) override;
    void itemDropped(const SourceDetails& details) override;
    void itemDragMove(const SourceDetails& details) override;
    void itemDragExit(const SourceDetails& details) override;

private:
    class BoardContent : public juce::Component
    {
    public:
        void resized() override {}
    };

    class AmpMarker : public juce::Component
    {
    public:
        void paint(juce::Graphics& g) override;
    };

    class RackIconButton : public juce::Button
    {
    public:
        RackIconButton(const juce::String& effectId, const juce::String& name);
        void paintButton(juce::Graphics& g, bool highlighted, bool down) override;

    private:
        juce::String id;
        juce::String label;
    };

    int indexAtX(int x) const;
    int chainIndexForEffectId(const juce::String& id) const;
    juce::Rectangle<int> boardArea() const;
    juce::Rectangle<int> rackArea() const;
    void pullUpPedal(const juce::String& id);
    void rackPedal(const juce::String& id);
    void requestRebuild();
    bool dropOverRack(const SourceDetails& details) const;

    AudioEngine& engine;
    juce::Viewport viewport;
    BoardContent content;
    AmpMarker ampMarker;
    juce::Label rackLabel { {}, "RACK" };
    std::vector<std::unique_ptr<PedalComponent>> pedals;
    std::vector<std::unique_ptr<RackIconButton>> rack;
    int hoverIndex = -1;
    bool hoverRack = false;
    bool rebuildPending = false;
};
