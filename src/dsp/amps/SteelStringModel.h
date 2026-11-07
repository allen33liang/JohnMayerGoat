#pragma once

#include "AmpModel.h"
#include "TriodeStage.h"
#include "ToneStack.h"
#include "PowerAmp.h"

class SteelStringModel : public AmpModel
{
public:
    juce::String getId() const override { return "steelString"; }
    juce::String getDisplayName() const override { return "Steel String"; }
    std::vector<ParamInfo> getParamList() const override;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    void process(float* samples, int numSamples) override;
    void setParameter(const juce::String& id, float value) override;
    float getParameter(const juce::String& id) const override;
    void restoreDefaults() override;

private:
    void updateControls();

    std::atomic<float> volume { 0.5f };
    std::atomic<float> treble { 0.55f };
    std::atomic<float> middle { 0.58f };
    std::atomic<float> bass { 0.52f };
    std::atomic<float> bright { 0.0f };

    juce::SmoothedValue<float> volSm, trebleSm, midSm, bassSm;
    TriodeStage v1, v2, v3;
    ToneStack stack;
    PowerAmp power;
    float brightZ = 0, brightX = 0, brightA = 0;
    float lastT = -1, lastM = -1, lastB = -1, lastV = -1;
};
