#pragma once

#include "CabinetSim.h"
#include "EffectChain.h"
#include <JuceHeader.h>

class AudioEngine : public juce::AudioIODeviceCallback,
                    public juce::ChangeBroadcaster
{
public:
    AudioEngine();

    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    EffectChain& getChain() { return chain; }
    const EffectChain& getChain() const { return chain; }
    CabinetSim& getCabinet() { return cabinet; }
    const CabinetSim& getCabinet() const { return cabinet; }

    void setInputGain(float zeroToOne);
    void setOutputGain(float zeroToOne);
    float getInputGain() const { return inputGain.load(); }
    float getOutputGain() const { return outputGain.load(); }

    void setMasterBypass(bool shouldBypass);
    bool isMasterBypass() const { return masterBypass.load(); }

    void factoryReset();

    juce::var toJson() const;
    void fromJson(const juce::var& json);

    float getInputPeak() const { return inputPeak.exchange(0.0f); }
    float getOutputPeak() const { return outputPeak.exchange(0.0f); }

private:
    EffectChain chain;
    CabinetSim cabinet;
    juce::AudioBuffer<float> workBuffer;

    std::atomic<float> inputGain { 0.7f };
    std::atomic<float> outputGain { 0.72f };
    std::atomic<bool> masterBypass { false };
    mutable std::atomic<float> inputPeak { 0.0f };
    mutable std::atomic<float> outputPeak { 0.0f };

    juce::SmoothedValue<float> inSm, outSm;
    double sampleRate = 44100.0;
};
