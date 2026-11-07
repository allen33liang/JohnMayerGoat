#pragma once

#include "Effect.h"
#include "DspUtils.h"

class TapeDelay : public Effect
{
public:
    juce::String getId() const override { return "delay"; }
    juce::String getDisplayName() const override { return "Echoplex"; }
    juce::Colour getColour() const override { return juce::Colour(0xff111111); }
    std::vector<ParamInfo> getParamList() const override;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void restoreDefaults() override;
    void setParameter(const juce::String& id, float value) override;
    float getParameter(const juce::String& id) const override;

private:
    std::atomic<float> time { 0.35f };
    std::atomic<float> feedback { 0.4f };
    std::atomic<float> mix { 0.35f };

    juce::SmoothedValue<float> timeSm, fbSm, mixSm;
    std::array<dsp_utils::DelayLine, dsp_utils::kMaxChannels> delays;
    std::array<dsp_utils::OnePoleLowpass, dsp_utils::kMaxChannels> tapeLp;
};
