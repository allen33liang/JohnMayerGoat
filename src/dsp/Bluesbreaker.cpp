#include "Bluesbreaker.h"

std::vector<ParamInfo> Bluesbreaker::getParamList() const
{
    return { { "drive", "Gain", 0.35f }, { "tone", "Tone", 0.5f }, { "level", "Volume", 0.65f } };
}

void Bluesbreaker::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;
    driveSm.reset(sampleRate, 0.03);
    toneSm.reset(sampleRate, 0.03);
    levelSm.reset(sampleRate, 0.03);
    driveSm.setCurrentAndTargetValue(drive.load());
    toneSm.setCurrentAndTargetValue(tone.load());
    levelSm.setCurrentAndTargetValue(level.load());

    oversampling.initProcessing((size_t) juce::jmax(1, samplesPerBlock));
    const auto spec = dsp_utils::makeSpec(sampleRate, samplesPerBlock);
    dsp_utils::ensureCoef(hipass);
    dsp_utils::ensureCoef(toneShelf);
    hipass.prepare(spec);
    toneShelf.prepare(spec);
    dsp_utils::assignCoef(hipass, juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass((float) sampleRate, 80.0f, 0.707f));
    lastTone = -1.0f;
    updateFilters();
    reset();
}

void Bluesbreaker::reset()
{
    oversampling.reset();
    hipass.reset();
    toneShelf.reset();
}

void Bluesbreaker::updateFilters()
{
    const float toneN = toneSm.getCurrentValue();
    if (std::abs(toneN - lastTone) <= 1.0e-4f)
        return;
    const float tGain = juce::Decibels::decibelsToGain(juce::jmap(toneN, 0.0f, 1.0f, -6.0f, 6.0f));
    dsp_utils::assignCoef(toneShelf, juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf((float) currentSampleRate, 2500.0f, 0.7f, tGain));
    lastTone = toneN;
}

void Bluesbreaker::process(juce::AudioBuffer<float>& buffer)
{
    driveSm.setTargetValue(drive.load());
    toneSm.setTargetValue(tone.load());
    levelSm.setTargetValue(level.load());

    const int n = buffer.getNumSamples();
    auto* p = buffer.getWritePointer(0);

    juce::dsp::AudioBlock<float> block(buffer);
    auto mono = block.getSingleChannelBlock(0);
    auto osBlock = oversampling.processSamplesUp(mono);
    auto* os = osBlock.getChannelPointer(0);
    const int osN = (int) osBlock.getNumSamples();
    const int stride = juce::jmax(1, osN / juce::jmax(1, n));

    float d = juce::Decibels::decibelsToGain(juce::jmap(driveSm.getCurrentValue(), 0.0f, 1.0f, 0.0f, 10.0f));
    for (int i = 0; i < osN; ++i)
    {
        if ((i % stride) == 0)
            d = juce::Decibels::decibelsToGain(juce::jmap(driveSm.getNextValue(), 0.0f, 1.0f, 0.0f, 10.0f));
        os[i] = 0.82f * os[i] + 0.18f * dsp_utils::softClip(os[i] * (1.0f + d * 0.7f));
    }
    oversampling.processSamplesDown(mono);

    for (int i = 0; i < n; ++i)
    {
        const float lvl = juce::jmap(levelSm.getNextValue(), 0.0f, 1.0f, 0.35f, 1.6f);
        toneSm.getNextValue();
        updateFilters();
        float s = hipass.processSample(p[i]);
        p[i] = toneShelf.processSample(s) * lvl;
    }
}

void Bluesbreaker::restoreDefaults()
{
    drive.store(0.35f);
    tone.store(0.5f);
    level.store(0.65f);
    setBypassed(true);
}

void Bluesbreaker::setParameter(const juce::String& id, float value)
{
    value = clamp01(value);
    if (id == "drive") drive.store(value);
    else if (id == "tone") tone.store(value);
    else if (id == "level") level.store(value);
}

float Bluesbreaker::getParameter(const juce::String& id) const
{
    if (id == "drive") return drive.load();
    if (id == "tone") return tone.load();
    if (id == "level") return level.load();
    return 0.0f;
}
