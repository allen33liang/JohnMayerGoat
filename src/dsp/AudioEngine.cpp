#include "AudioEngine.h"
#include "DspUtils.h"
#include <cmath>

AudioEngine::AudioEngine() = default;

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    sampleRate = device->getCurrentSampleRate();
    const int block = device->getCurrentBufferSizeSamples();
    workBuffer.setSize(2, juce::jmax(block, 32), false, false, true);
    inSm.reset(sampleRate, 0.02);
    outSm.reset(sampleRate, 0.02);
    inSm.setCurrentAndTargetValue(inputGain.load());
    outSm.setCurrentAndTargetValue(outputGain.load());
    chain.prepare(sampleRate, block);
    cabinet.prepare(sampleRate, block);
}

void AudioEngine::audioDeviceStopped()
{
    chain.reset();
    cabinet.reset();
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                   int numInputChannels,
                                                   float* const* outputChannelData,
                                                   int numOutputChannels,
                                                   int numSamples,
                                                   const juce::AudioIODeviceCallbackContext&)
{
    const int workChans = juce::jmax(2, numOutputChannels);
    if (workBuffer.getNumChannels() < workChans || workBuffer.getNumSamples() < numSamples)
        return;

    workBuffer.clear();

    const int inChans = juce::jmax(0, numInputChannels);
    if (inChans > 0 && inputChannelData != nullptr && inputChannelData[0] != nullptr)
    {
        workBuffer.copyFrom(0, 0, inputChannelData[0], numSamples);
        if (inChans > 1 && inputChannelData[1] != nullptr)
            workBuffer.copyFrom(1, 0, inputChannelData[1], numSamples);
        else
            workBuffer.copyFrom(1, 0, workBuffer, 0, 0, numSamples);
    }

    inSm.setTargetValue(inputGain.load());
    outSm.setTargetValue(outputGain.load());

    float* ch0 = workBuffer.getWritePointer(0);
    float* ch1 = workBuffer.getWritePointer(1);

    float inPk = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        const float g = inSm.getNextValue();
        ch0[i] *= g;
        ch1[i] *= g;
        inPk = juce::jmax(inPk, std::abs(ch0[i]), std::abs(ch1[i]));
    }
    inputPeak.store(inPk);

    if (! masterBypass.load())
        chain.process(workBuffer, &cabinet);

    ch0 = workBuffer.getWritePointer(0);
    ch1 = workBuffer.getWritePointer(1);

    float outPk = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        const float g = outSm.getNextValue();
        ch0[i] = dsp_utils::safetyLimit(ch0[i] * g);
        ch1[i] = dsp_utils::safetyLimit(ch1[i] * g);
        outPk = juce::jmax(outPk, std::abs(ch0[i]), std::abs(ch1[i]));
    }
    outputPeak.store(outPk);

    for (int ch = 0; ch < numOutputChannels; ++ch)
    {
        if (outputChannelData[ch] == nullptr)
            continue;
        const int src = juce::jmin(ch, 1);
        juce::FloatVectorOperations::copy(outputChannelData[ch], workBuffer.getReadPointer(src), numSamples);
    }
}

void AudioEngine::setInputGain(float zeroToOne)
{
    inputGain.store(juce::jlimit(0.0f, 1.0f, zeroToOne));
}

void AudioEngine::setOutputGain(float zeroToOne)
{
    outputGain.store(juce::jlimit(0.0f, 1.0f, zeroToOne));
}

void AudioEngine::setMasterBypass(bool shouldBypass)
{
    masterBypass.store(shouldBypass);
}

void AudioEngine::factoryReset()
{
    for (auto& fx : chain.getOrderedEffects())
    {
        fx->restoreDefaults();
        fx->setOnBoard(fx->getId() == "preamp");
    }
    chain.restoreDefaultOrder();
    cabinet.restoreDefaults();
    setInputGain(0.7f);
    setOutputGain(0.72f);
    setMasterBypass(false);
    sendChangeMessage();
}

juce::var AudioEngine::toJson() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty("name", "Untitled");
    obj->setProperty("inputGain", inputGain.load());
    obj->setProperty("outputGain", outputGain.load());
    obj->setProperty("masterBypass", masterBypass.load());
    obj->setProperty("cabinet", cabinet.toJson());
    obj->setProperty("chain", chain.toJson());
    return juce::var(obj);
}

void AudioEngine::fromJson(const juce::var& json)
{
    if (auto* obj = json.getDynamicObject())
    {
        if (obj->hasProperty("inputGain"))
            setInputGain((float) (double) obj->getProperty("inputGain"));
        if (obj->hasProperty("outputGain"))
            setOutputGain((float) (double) obj->getProperty("outputGain"));
        if (obj->hasProperty("masterBypass"))
            setMasterBypass((bool) obj->getProperty("masterBypass"));
        if (obj->hasProperty("cabinet"))
            cabinet.fromJson(obj->getProperty("cabinet"));
        if (obj->hasProperty("chain"))
            chain.fromJson(obj->getProperty("chain"));
    }
    sendChangeMessage();
}
