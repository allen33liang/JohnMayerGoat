#pragma once

#include "Effect.h"
#include "DspUtils.h"

class Shredmaster : public Effect
{
public:
    juce::String getId() const override { return "shredmaster"; }
    juce::String getDisplayName() const override { return "Shredmaster"; }
    juce::Colour getColour() const override { return juce::Colour(0xff111111); }
    std::vector<ParamInfo> getParamList() const override;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void restoreDefaults() override;
    void setParameter(const juce::String& id, float value) override;
    float getParameter(const juce::String& id) const override;

private:
    void updateFilters();

    std::atomic<float> gain { 0.65f };
    std::atomic<float> bass { 0.5f };
    std::atomic<float> treble { 0.5f };
    std::atomic<float> volume { 0.5f };

    juce::SmoothedValue<float> gainSm, bassSm, trebleSm, volSm;
    juce::dsp::Oversampling<float> oversampling { 1, 3, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
    juce::dsp::IIR::Filter<float> hipass, preLp, postLp, bassFilter, scoop, trebleFilter;
    dsp_utils::OnePoleHighpass dcBlock;
    float lastBass = -1.0f, lastTreble = -1.0f;
};
