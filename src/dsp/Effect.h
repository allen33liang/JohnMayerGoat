#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <vector>

struct ParamInfo
{
    juce::String id;
    juce::String label;
    float defaultValue = 0.5f;
    bool isSwitch = false;
};

class Effect
{
public:
    virtual ~Effect() = default;

    virtual juce::String getId() const = 0;
    virtual juce::String getDisplayName() const = 0;
    virtual juce::Colour getColour() const = 0;
    virtual std::vector<ParamInfo> getParamList() const = 0;

    virtual void prepare(double sampleRate, int samplesPerBlock) = 0;
    virtual void reset() = 0;
    virtual void process(juce::AudioBuffer<float>& buffer) = 0;
    virtual void restoreDefaults() = 0;

    void setBypassed(bool shouldBypass) noexcept { bypassed.store(shouldBypass); }
    bool isBypassed() const noexcept { return bypassed.load(); }

    void setOnBoard(bool shouldBeOnBoard) noexcept { onBoard.store(shouldBeOnBoard); }
    bool isOnBoard() const noexcept { return onBoard.load(); }

    virtual void setParameter(const juce::String& id, float value) = 0;
    virtual float getParameter(const juce::String& id) const = 0;

    virtual juce::var toJson() const;
    virtual void fromJson(const juce::var& json);

    void setPrepared(double sampleRate, int samplesPerBlock);
    void processWithFade(juce::AudioBuffer<float>& buffer);

    static bool isTimeBasedId(const juce::String& id) noexcept
    {
        return id == "chorus" || id == "delay" || id == "slapback" || id == "reverb";
    }

protected:
    std::atomic<bool> bypassed { true };
    std::atomic<bool> onBoard { false };
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    static float clamp01(float v) noexcept { return juce::jlimit(0.0f, 1.0f, v); }

private:
    juce::AudioBuffer<float> dryBuffer;
    juce::SmoothedValue<float> wetSm;
};
