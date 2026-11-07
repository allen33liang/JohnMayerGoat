#pragma once

#include "Effect.h"
#include "DspUtils.h"

class Chorus : public Effect
{
public:
    juce::String getId() const override { return "chorus"; }
    juce::String getDisplayName() const override { return "Chorus"; }
    juce::Colour getColour() const override { return juce::Colour(0xff4ec2e8); }
    std::vector<ParamInfo> getParamList() const override;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void restoreDefaults() override;
    void setParameter(const juce::String& id, float value) override;
    float getParameter(const juce::String& id) const override;

private:
    std::atomic<float> rate { 0.35f };
    std::atomic<float> depth { 0.45f };
    std::atomic<float> mix { 0.5f };

    juce::SmoothedValue<float> rateSm, depthSm, mixSm;
    std::array<dsp_utils::DelayLine, dsp_utils::kMaxChannels> delays;
    float lfoPhase = 0.0f;
};
