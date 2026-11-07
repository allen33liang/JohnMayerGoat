#pragma once

#include "Effect.h"
#include "DspUtils.h"

class SlapbackDelay : public Effect
{
public:
    juce::String getId() const override { return "slapback"; }
    juce::String getDisplayName() const override { return "Aqua-Puss"; }
    juce::Colour getColour() const override { return juce::Colour(0xff1aa7d0); }
    std::vector<ParamInfo> getParamList() const override;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void restoreDefaults() override;
    void setParameter(const juce::String& id, float value) override;
    float getParameter(const juce::String& id) const override;

private:
    std::atomic<float> delay { 0.32f };
    std::atomic<float> feedback { 0.22f };
    std::atomic<float> blend { 0.4f };

    juce::SmoothedValue<float> delaySm, fbSm, blendSm;
    std::array<dsp_utils::DelayLine, dsp_utils::kMaxChannels> delays;
    std::array<dsp_utils::OnePoleLowpass, dsp_utils::kMaxChannels> bbdLp;
};
