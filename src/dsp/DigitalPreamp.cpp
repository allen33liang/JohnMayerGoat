#include "DigitalPreamp.h"

DigitalPreamp::DigitalPreamp()
{
    bypassed.store(false);
    onBoard.store(true);
}

juce::String DigitalPreamp::modelName(int index)
{
    return index == 1 ? "Steel String" : "Deluxe Reverb";
}

AmpModel* DigitalPreamp::modelFromIndex(int index) const
{
    if (index == 1)
        return const_cast<SteelStringModel*>(&steel);
    return const_cast<DeluxeReverbModel*>(&deluxe);
}

AmpModel* DigitalPreamp::getActiveModel() const
{
    return modelFromIndex(modelIndex.load());
}

AmpModel* DigitalPreamp::getModelAt(int index) const
{
    return modelFromIndex(index);
}

std::vector<ParamInfo> DigitalPreamp::getParamList() const
{
    return getActiveModel()->getParamList();
}

void DigitalPreamp::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;
    oversampling.initProcessing((size_t) juce::jmax(1, samplesPerBlock));
    const double osRate = sampleRate * oversampling.getOversamplingFactor();
    const int osBlock = samplesPerBlock * (int) oversampling.getOversamplingFactor();
    deluxe.prepare(osRate, osBlock);
    steel.prepare(osRate, osBlock);
    fadeScratchN = juce::jmax(osBlock * 2, 8192);
    fadeScratch.allocate((size_t) fadeScratchN, true);
    fadeLength = juce::jmax(1, (int) std::round(0.04 * sampleRate));
    fadeSamplesLeft = 0;
    liveIndex = juce::jlimit(0, 1, modelIndex.load());
    reset();
}

void DigitalPreamp::reset()
{
    oversampling.reset();
    deluxe.reset();
    steel.reset();
    fadeSamplesLeft = 0;
}

void DigitalPreamp::setModelIndex(int index)
{
    index = juce::jlimit(0, 1, index);
    const int cur = modelIndex.load();
    if (index == cur)
        return;
    fadeFrom = liveIndex;
    fadeSamplesLeft = fadeLength;
    modelIndex.store(index);
}

void DigitalPreamp::process(juce::AudioBuffer<float>& buffer)
{
    const int requested = juce::jlimit(0, 1, modelIndex.load());
    if (requested != liveIndex && fadeSamplesLeft <= 0)
    {
        fadeFrom = liveIndex;
        fadeSamplesLeft = fadeLength;
        liveIndex = requested;
    }
    else if (requested != liveIndex)
    {
        liveIndex = requested;
    }

    juce::dsp::AudioBlock<float> block(buffer);
    auto mono = block.getSingleChannelBlock(0);
    auto osBlock = oversampling.processSamplesUp(mono);
    auto* os = osBlock.getChannelPointer(0);
    const int osN = (int) osBlock.getNumSamples();

    auto* active = modelFromIndex(liveIndex);
    if (fadeSamplesLeft > 0 && fadeFrom != liveIndex && fadeScratchN >= osN)
    {
        juce::FloatVectorOperations::copy(fadeScratch.get(), os, osN);
        modelFromIndex(fadeFrom)->process(fadeScratch.get(), osN);
        active->process(os, osN);

        const float twoPiQuarter = 0.5f * juce::MathConstants<float>::pi;
        for (int i = 0; i < osN; ++i)
        {
            const float t = 1.0f - (float) juce::jmax(0, fadeSamplesLeft) / (float) fadeLength;
            const float wNew = std::sin(juce::jlimit(0.0f, 1.0f, t) * twoPiQuarter);
            const float wOld = std::cos(juce::jlimit(0.0f, 1.0f, t) * twoPiQuarter);
            os[i] = fadeScratch[i] * wOld + os[i] * wNew;
            if ((i % 4) == 0 && fadeSamplesLeft > 0)
                --fadeSamplesLeft;
        }
        if (fadeSamplesLeft < 0)
            fadeSamplesLeft = 0;
    }
    else
    {
        active->process(os, osN);
        fadeSamplesLeft = 0;
    }

    oversampling.processSamplesDown(mono);
}

void DigitalPreamp::restoreDefaults()
{
    deluxe.restoreDefaults();
    steel.restoreDefaults();
    setModelIndex(0);
    setBypassed(false);
    setOnBoard(true);
}

void DigitalPreamp::setParameter(const juce::String& id, float value)
{
    if (id == "model")
    {
        setModelIndex((int) std::round(value));
        return;
    }
    getActiveModel()->setParameter(id, value);
}

float DigitalPreamp::getParameter(const juce::String& id) const
{
    if (id == "model")
        return (float) modelIndex.load();
    return getActiveModel()->getParameter(id);
}

juce::var DigitalPreamp::toJson() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("id", getId());
    obj->setProperty("bypassed", isBypassed());
    obj->setProperty("onBoard", isOnBoard());
    obj->setProperty("model", modelFromIndex(modelIndex.load())->getId());
    auto* models = new juce::DynamicObject();
    models->setProperty("deluxe", deluxe.paramsToJson());
    models->setProperty("steelString", steel.paramsToJson());
    obj->setProperty("models", juce::var(models));
    obj->setProperty("params", getActiveModel()->paramsToJson());
    return juce::var(obj);
}

void DigitalPreamp::fromJson(const juce::var& json)
{
    if (auto* obj = json.getDynamicObject())
    {
        if (obj->hasProperty("bypassed"))
            setBypassed((bool) obj->getProperty("bypassed"));
        if (obj->hasProperty("onBoard"))
            setOnBoard((bool) obj->getProperty("onBoard"));

        auto models = obj->getProperty("models");
        if (auto* mobj = models.getDynamicObject())
        {
            if (mobj->hasProperty("deluxe")) deluxe.paramsFromJson(mobj->getProperty("deluxe"));
            if (mobj->hasProperty("steelString")) steel.paramsFromJson(mobj->getProperty("steelString"));
        }

        const bool hasModel = obj->hasProperty("model");
        const auto modelId = obj->getProperty("model").toString();
        int idx = (modelId == "steelString") ? 1 : 0;

        auto params = obj->getProperty("params");
        if (auto* pobj = params.getDynamicObject())
        {
            if (! hasModel)
            {
                if (pobj->hasProperty("gain"))
                    deluxe.setParameter("volume", (float) (double) pobj->getProperty("gain"));
                if (pobj->hasProperty("bass"))
                    deluxe.setParameter("bass", (float) (double) pobj->getProperty("bass"));
                if (pobj->hasProperty("treble"))
                    deluxe.setParameter("treble", (float) (double) pobj->getProperty("treble"));
                idx = 0;
            }
            else
            {
                modelFromIndex(idx)->paramsFromJson(params);
            }
        }
        setModelIndex(idx);
    }
}
