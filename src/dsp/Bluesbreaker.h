#pragma once

#include "Effect.h"
#include "DspUtils.h"

class Bluesbreaker : public Effect
{
public:
    juce::String getId() const override { return "bluesbreaker"; }
    juce::String getDisplayName() const override { return "Blues Breaker"; }
    juce::Colour getColour() const override { return juce::Colour(0xff1a1614); }
    std::vector<ParamInfo> getParamList() const override;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void restoreDefaults() override;
    void setParameter(const juce::String& id, float value) override;
    float getParameter(const juce::String& id) const override;

private:
    void updateFilters();

    std::atomic<float> drive { 0.35f };
    std::atomic<float> tone { 0.5f };
    std::atomic<float> level { 0.65f };

    juce::SmoothedValue<float> driveSm, toneSm, levelSm;
    juce::dsp::Oversampling<float> oversampling { 1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
    juce::dsp::IIR::Filter<float> hipass, toneShelf;
    float lastTone = -1.0f;
};
