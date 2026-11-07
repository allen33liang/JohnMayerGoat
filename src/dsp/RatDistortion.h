#pragma once

#include "Effect.h"
#include "DspUtils.h"

class RatDistortion : public Effect
{
public:
    juce::String getId() const override { return "rat"; }
    juce::String getDisplayName() const override { return "RAT"; }
    juce::Colour getColour() const override { return juce::Colour(0xff141414); }
    std::vector<ParamInfo> getParamList() const override;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void restoreDefaults() override;
    void setParameter(const juce::String& id, float value) override;
    float getParameter(const juce::String& id) const override;

private:
    void updateFilters();

    std::atomic<float> distortion { 0.5f };
    std::atomic<float> filter { 0.6f };
    std::atomic<float> volume { 0.5f };

    juce::SmoothedValue<float> distSm, filterSm, volSm;
    juce::dsp::Oversampling<float> oversampling { 1, 3, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
    juce::dsp::IIR::Filter<float> bassCut, preLp, gbWlp, postLp, filterLp;
    dsp_utils::OnePoleHighpass dcBlock;
    float lastFilter = -1.0f, lastDist = -1.0f;
};
