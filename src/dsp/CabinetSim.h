#pragma once

#include "DspUtils.h"
#include <JuceHeader.h>
#include <atomic>
#include <array>

class CabinetSim
{
public:
    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    void process(juce::AudioBuffer<float>& buffer);

    void setEnabled(bool shouldEnable) noexcept { enabled.store(shouldEnable); }
    bool isEnabled() const noexcept { return enabled.load(); }

    void setBody(float zeroToOne);
    void setAir(float zeroToOne);
    void setLevel(float zeroToOne);
    float getBody() const { return body.load(); }
    float getAir() const { return air.load(); }
    float getLevel() const { return level.load(); }

    void restoreDefaults();
    bool loadImpulseFromFile(const juce::File& file);
    void clearImpulse();
    void setIrFolder(const juce::File& folder);
    juce::File getIrFolder() const;
    juce::String getLoadedIrName() const { return loadedIrName; }
    juce::StringArray listImpulseFiles() const;
    bool isUsingIr() const { return usingIr.load(); }
    static juce::File defaultIrFolder();
    static float levelToDecibels(float zeroToOne) noexcept
    {
        return juce::jmap(zeroToOne, 0.0f, 1.0f, -12.0f, 12.0f);
    }

    juce::var toJson() const;
    void fromJson(const juce::var& json);

private:
    void updateFilters();
    void processFallback(float* p, int n);
    void updateTargetGain() noexcept;
    float computeMakeupGain(const juce::AudioBuffer<float>& ir) const;

    std::atomic<bool> enabled { true };
    std::atomic<bool> usingIr { false };
    std::atomic<float> body { 0.55f };
    std::atomic<float> air { 0.45f };
    std::atomic<float> level { 0.5f };
    std::atomic<float> makeupGain { 1.0f };
    std::atomic<float> targetGain { 1.0f };

    juce::SmoothedValue<float> bodySm, airSm, gainSm;
    juce::dsp::Convolution convolution { juce::dsp::Convolution::Latency { 0 } };
    juce::dsp::IIR::Filter<float> hpf, bodyPeak, scoop, presence, lpf;
    juce::dsp::ProcessSpec processSpec { 44100.0, 512, 1 };
    juce::File irFolder;
    juce::String loadedIrName, pendingIrName;
    double currentSampleRate = 44100.0;
    float lastBody = -1.0f, lastAir = -1.0f;
};
