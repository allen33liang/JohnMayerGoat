#pragma once

#include "dsp/Effect.h"
#include "dsp/DspUtils.h"
#include <atomic>

class AmpModel
{
public:
    virtual ~AmpModel() = default;

    virtual juce::String getId() const = 0;
    virtual juce::String getDisplayName() const = 0;
    virtual std::vector<ParamInfo> getParamList() const = 0;

    virtual void prepare(double sampleRate, int samplesPerBlock) = 0;
    virtual void reset() = 0;
    virtual void process(float* samples, int numSamples) = 0;

    virtual void setParameter(const juce::String& id, float value) = 0;
    virtual float getParameter(const juce::String& id) const = 0;
    virtual void restoreDefaults() = 0;

    juce::var paramsToJson() const
    {
        auto* obj = new juce::DynamicObject();
        for (const auto& p : getParamList())
            obj->setProperty(p.id, getParameter(p.id));
        return juce::var(obj);
    }

    void paramsFromJson(const juce::var& json)
    {
        if (auto* obj = json.getDynamicObject())
        {
            for (const auto& p : getParamList())
                if (obj->hasProperty(p.id))
                    setParameter(p.id, (float) (double) obj->getProperty(p.id));
        }
    }

protected:
    static float clamp01(float v) noexcept { return juce::jlimit(0.0f, 1.0f, v); }
    double sr = 44100.0;
    int block = 512;
};
