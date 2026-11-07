#pragma once

#include "Effect.h"
#include "DspUtils.h"
#include "amps/AmpModel.h"
#include "amps/DeluxeReverbModel.h"
#include "amps/SteelStringModel.h"

class DigitalPreamp : public Effect
{
public:
    DigitalPreamp();

    juce::String getId() const override { return "preamp"; }
    juce::String getDisplayName() const override { return "Digital Preamp"; }
    juce::Colour getColour() const override { return juce::Colour(0xff8b1e1e); }
    std::vector<ParamInfo> getParamList() const override;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void restoreDefaults() override;
    void setParameter(const juce::String& id, float value) override;
    float getParameter(const juce::String& id) const override;
    juce::var toJson() const override;
    void fromJson(const juce::var& json) override;

    void setModelIndex(int index);
    int getModelIndex() const { return modelIndex.load(); }
    AmpModel* getActiveModel() const;
    AmpModel* getModelAt(int index) const;
    static int modelCount() noexcept { return 2; }
    static juce::String modelName(int index);

private:
    AmpModel* modelFromIndex(int index) const;

    DeluxeReverbModel deluxe;
    SteelStringModel steel;
    std::atomic<int> modelIndex { 0 };
    int liveIndex = 0;
    int fadeFrom = 0;
    int fadeSamplesLeft = 0;
    int fadeLength = 1;
    juce::dsp::Oversampling<float> oversampling { 1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };
    juce::HeapBlock<float> fadeScratch;
    int fadeScratchN = 0;
};
