#pragma once

#include "AmpModel.h"
#include "TriodeStage.h"
#include "ToneStack.h"
#include "PowerAmp.h"

class DeluxeReverbModel : public AmpModel
{
public:
    juce::String getId() const override { return "deluxe"; }
    juce::String getDisplayName() const override { return "Deluxe Reverb"; }
    std::vector<ParamInfo> getParamList() const override;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    void process(float* samples, int numSamples) override;
    void setParameter(const juce::String& id, float value) override;
    float getParameter(const juce::String& id) const override;
    void restoreDefaults() override;

private:
    void updateControls();

    std::atomic<float> volume { 0.45f };
    std::atomic<float> treble { 0.62f };
    std::atomic<float> bass { 0.48f };
    std::atomic<float> bright { 0.0f };

    juce::SmoothedValue<float> volSm, trebleSm, bassSm;
    TriodeStage v1, v2;
    ToneStack stack;
    PowerAmp power;
    float brightHpA = 0.0f, brightZ = 0.0f, brightX = 0.0f;
    float lastT = -1.0f, lastB = -1.0f, lastV = -1.0f, lastBright = -1.0f;
};
