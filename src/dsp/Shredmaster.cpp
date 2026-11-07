#include "Shredmaster.h"

std::vector<ParamInfo> Shredmaster::getParamList() const
{
    return {
        { "gain", "Gain", 0.65f },
        { "bass", "Bass", 0.5f },
        { "treble", "Treble", 0.5f },
        { "volume", "Volume", 0.5f }
    };
}

void Shredmaster::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;
    for (auto* sm : { &gainSm, &bassSm, &trebleSm, &volSm })
        sm->reset(sampleRate, 0.03);
    gainSm.setCurrentAndTargetValue(gain.load());
    bassSm.setCurrentAndTargetValue(bass.load());
    trebleSm.setCurrentAndTargetValue(treble.load());
    volSm.setCurrentAndTargetValue(volume.load());

    oversampling.initProcessing((size_t) juce::jmax(1, samplesPerBlock));
    const float sr = (float) sampleRate;
    const float osRate = sr * 8.0f;
    const auto osSpec = dsp_utils::makeSpec((double) osRate, samplesPerBlock * 8);
    const auto baseSpec = dsp_utils::makeSpec(sampleRate, samplesPerBlock);
    for (auto* f : { &hipass, &preLp, &postLp, &bassFilter, &scoop, &trebleFilter })
        dsp_utils::ensureCoef(*f);
    hipass.prepare(osSpec);
    preLp.prepare(osSpec);
    postLp.prepare(osSpec);
    bassFilter.prepare(baseSpec);
    scoop.prepare(baseSpec);
    trebleFilter.prepare(baseSpec);
    dcBlock.setCutoff(osRate, 10.0f);
    dsp_utils::assignCoef(hipass, juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderHighPass(osRate, 55.0f));
    dsp_utils::assignCoef(preLp, juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(osRate, 3600.0f, 0.65f));
    dsp_utils::assignCoef(postLp, juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(osRate, 4500.0f, 0.7f));
    dsp_utils::assignCoef(scoop, juce::dsp::IIR::ArrayCoefficients<float>::makePeakFilter(sr, 700.0f, 0.75f, juce::Decibels::decibelsToGain(2.0f)));
    lastBass = lastTreble = -1.0f;
    updateFilters();
    reset();
}

void Shredmaster::reset()
{
    oversampling.reset();
    hipass.reset();
    preLp.reset();
    postLp.reset();
    bassFilter.reset();
    scoop.reset();
    trebleFilter.reset();
    dcBlock.reset();
}

void Shredmaster::updateFilters()
{
    const float bassN = bassSm.getCurrentValue();
    const float trebleN = trebleSm.getCurrentValue();
    const float sr = (float) currentSampleRate;
    if (std::abs(bassN - lastBass) > 1.0e-4f)
    {
        const float bassGain = juce::Decibels::decibelsToGain(juce::jmap(bassN, 0.0f, 1.0f, -4.0f, 10.0f));
        dsp_utils::assignCoef(bassFilter, juce::dsp::IIR::ArrayCoefficients<float>::makeLowShelf(sr, 180.0f, 0.7f, bassGain));
        lastBass = bassN;
    }
    if (std::abs(trebleN - lastTreble) > 1.0e-4f)
    {
        const float trebleGain = juce::Decibels::decibelsToGain(juce::jmap(trebleN, 0.0f, 1.0f, -6.0f, 4.0f));
        dsp_utils::assignCoef(trebleFilter, juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf(sr, 2500.0f, 0.7f, trebleGain));
        lastTreble = trebleN;
    }
}

void Shredmaster::process(juce::AudioBuffer<float>& buffer)
{
    gainSm.setTargetValue(gain.load());
    bassSm.setTargetValue(bass.load());
    trebleSm.setTargetValue(treble.load());
    volSm.setTargetValue(volume.load());

    const int n = buffer.getNumSamples();
    auto* p = buffer.getWritePointer(0);
    juce::dsp::AudioBlock<float> block(buffer);
    auto mono = block.getSingleChannelBlock(0);
    auto osBlock = oversampling.processSamplesUp(mono);
    auto* os = osBlock.getChannelPointer(0);
    const int osN = (int) osBlock.getNumSamples();
    const int stride = juce::jmax(1, osN / juce::jmax(1, n));
    float g = juce::Decibels::decibelsToGain(juce::jmap(gainSm.getCurrentValue(), 0.0f, 1.0f, 8.0f, 30.0f));
    for (int i = 0; i < osN; ++i)
    {
        if ((i % stride) == 0)
            g = juce::Decibels::decibelsToGain(juce::jmap(gainSm.getNextValue(), 0.0f, 1.0f, 8.0f, 30.0f));
        float s = hipass.processSample(os[i]);
        s = preLp.processSample(s);
        s = dsp_utils::softClip(s * g * 0.62f);
        s = dcBlock.process(s);
        os[i] = postLp.processSample(s);
    }
    oversampling.processSamplesDown(mono);

    for (int i = 0; i < n; ++i)
    {
        const float lvl = juce::jmap(volSm.getNextValue(), 0.0f, 1.0f, 0.35f, 1.3f);
        bassSm.getNextValue();
        trebleSm.getNextValue();
        updateFilters();
        float s = bassFilter.processSample(p[i]);
        s = scoop.processSample(s);
        p[i] = trebleFilter.processSample(s) * lvl;
    }
}

void Shredmaster::restoreDefaults()
{
    gain.store(0.65f);
    bass.store(0.5f);
    treble.store(0.5f);
    volume.store(0.5f);
    setBypassed(true);
}

void Shredmaster::setParameter(const juce::String& id, float value)
{
    value = clamp01(value);
    if (id == "gain") gain.store(value);
    else if (id == "bass") bass.store(value);
    else if (id == "treble") treble.store(value);
    else if (id == "volume") volume.store(value);
}

float Shredmaster::getParameter(const juce::String& id) const
{
    if (id == "gain") return gain.load();
    if (id == "bass") return bass.load();
    if (id == "treble") return treble.load();
    if (id == "volume") return volume.load();
    return 0.0f;
}
