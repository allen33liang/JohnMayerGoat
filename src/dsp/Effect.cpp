#include "Effect.h"

juce::var Effect::toJson() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("id", getId());
    obj->setProperty("bypassed", isBypassed());
    obj->setProperty("onBoard", isOnBoard());

    auto* params = new juce::DynamicObject();
    for (const auto& p : getParamList())
        params->setProperty(p.id, getParameter(p.id));

    obj->setProperty("params", juce::var(params));
    return juce::var(obj);
}

void Effect::fromJson(const juce::var& json)
{
    if (auto* obj = json.getDynamicObject())
    {
        if (obj->hasProperty("bypassed"))
            setBypassed((bool) obj->getProperty("bypassed"));

        if (obj->hasProperty("onBoard"))
            setOnBoard((bool) obj->getProperty("onBoard"));
        else
            setOnBoard(! isBypassed() && getId() != "preamp");

        auto params = obj->getProperty("params");
        if (auto* pobj = params.getDynamicObject())
        {
            for (const auto& p : getParamList())
            {
                if (pobj->hasProperty(p.id))
                    setParameter(p.id, (float) (double) pobj->getProperty(p.id));
            }
        }
    }
}

void Effect::setPrepared(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;
    dryBuffer.setSize(2, juce::jmax(2048, samplesPerBlock), false, false, true);
    wetSm.reset(sampleRate, 0.01);
    wetSm.setCurrentAndTargetValue(bypassed.load() ? 0.0f : 1.0f);
    prepare(sampleRate, samplesPerBlock);
}

void Effect::processWithFade(juce::AudioBuffer<float>& buffer)
{
    const float target = bypassed.load() ? 0.0f : 1.0f;
    wetSm.setTargetValue(target);

    if (target == 0.0f && ! wetSm.isSmoothing())
        return;

    const int n = buffer.getNumSamples();
    const int chans = juce::jmin(buffer.getNumChannels(), dryBuffer.getNumChannels());
    const bool fading = wetSm.isSmoothing() || (target > 0.0f && wetSm.getCurrentValue() < 0.999f);

    if (fading && dryBuffer.getNumSamples() >= n)
    {
        for (int ch = 0; ch < chans; ++ch)
            dryBuffer.copyFrom(ch, 0, buffer, ch, 0, n);
    }

    if (target > 0.0f || wetSm.isSmoothing())
        process(buffer);

    if (! fading || dryBuffer.getNumSamples() < n)
        return;

    for (int i = 0; i < n; ++i)
    {
        const float w = wetSm.getNextValue();
        for (int ch = 0; ch < chans; ++ch)
        {
            auto* wet = buffer.getWritePointer(ch);
            wet[i] = dryBuffer.getReadPointer(ch)[i] * (1.0f - w) + wet[i] * w;
        }
    }
}
