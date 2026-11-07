#include "RatDistortion.h"

std::vector<ParamInfo> RatDistortion::getParamList() const
{
    return { { "distortion", "Distortion", 0.5f }, { "filter", "Filter", 0.6f }, { "volume", "Volume", 0.5f } };
}

void RatDistortion::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;
    distSm.reset(sampleRate, 0.03);
    filterSm.reset(sampleRate, 0.03);
    volSm.reset(sampleRate, 0.03);
    distSm.setCurrentAndTargetValue(distortion.load());
    filterSm.setCurrentAndTargetValue(filter.load());
    volSm.setCurrentAndTargetValue(volume.load());

    oversampling.initProcessing((size_t) juce::jmax(1, samplesPerBlock));
    const float sr = (float) sampleRate;
    const float osRate = sr * 8.0f;
    const auto osSpec = dsp_utils::makeSpec((double) osRate, samplesPerBlock * 8);
    const auto baseSpec = dsp_utils::makeSpec(sampleRate, samplesPerBlock);
    for (auto* f : { &bassCut, &preLp, &gbWlp, &postLp, &filterLp })
        dsp_utils::ensureCoef(*f);
    bassCut.prepare(osSpec);
    preLp.prepare(osSpec);
    gbWlp.prepare(osSpec);
    postLp.prepare(osSpec);
    filterLp.prepare(baseSpec);
    dcBlock.setCutoff(osRate, 15.0f);
    dsp_utils::assignCoef(bassCut, juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderHighPass(osRate, 120.0f));
    dsp_utils::assignCoef(preLp, juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(osRate, 3800.0f, 0.7f));
    dsp_utils::assignCoef(postLp, juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(osRate, 3200.0f, 0.7f));
    lastFilter = lastDist = -1.0f;
    updateFilters();
    reset();
}

void RatDistortion::reset()
{
    oversampling.reset();
    bassCut.reset();
    preLp.reset();
    gbWlp.reset();
    postLp.reset();
    filterLp.reset();
    dcBlock.reset();
}

void RatDistortion::updateFilters()
{
    const float filterN = filterSm.getCurrentValue();
    const float distN = distSm.getCurrentValue();
    const float sr = (float) currentSampleRate;
    const float osRate = sr * 8.0f;

    if (std::abs(distN - lastDist) > 1.0e-4f)
    {
        const float gainLin = juce::Decibels::decibelsToGain(juce::jmap(distN, 0.0f, 1.0f, 10.0f, 48.0f));
        const float gbwHz = juce::jlimit(1200.0f, 8000.0f, 1.0e6f / juce::jmax(12.0f, gainLin));
        dsp_utils::assignCoef(gbWlp, juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(osRate, gbwHz, 0.65f));
        lastDist = distN;
    }
    if (std::abs(filterN - lastFilter) > 1.0e-4f)
    {
        const float hz = juce::jmap(filterN, 0.0f, 1.0f, 10000.0f, 400.0f);
        dsp_utils::assignCoef(filterLp, juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(sr, juce::jmin(hz, sr * 0.45f), 0.7f));
        lastFilter = filterN;
    }
}

void RatDistortion::process(juce::AudioBuffer<float>& buffer)
{
    distSm.setTargetValue(distortion.load());
    filterSm.setTargetValue(filter.load());
    volSm.setTargetValue(volume.load());

    const int n = buffer.getNumSamples();
    auto* p = buffer.getWritePointer(0);
    juce::dsp::AudioBlock<float> block(buffer);
    auto mono = block.getSingleChannelBlock(0);
    auto osBlock = oversampling.processSamplesUp(mono);
    auto* os = osBlock.getChannelPointer(0);
    const int osN = (int) osBlock.getNumSamples();
    const int stride = juce::jmax(1, osN / juce::jmax(1, n));

    float d = juce::Decibels::decibelsToGain(juce::jmap(distSm.getCurrentValue(), 0.0f, 1.0f, 10.0f, 48.0f));
    for (int i = 0; i < osN; ++i)
    {
        if ((i % stride) == 0)
        {
            d = juce::Decibels::decibelsToGain(juce::jmap(distSm.getNextValue(), 0.0f, 1.0f, 10.0f, 48.0f));
            updateFilters();
        }
        float s = bassCut.processSample(os[i]);
        s = preLp.processSample(s);
        s = gbWlp.processSample(s * d * 0.18f);
        s = dsp_utils::softClip(s);
        s = dcBlock.process(s);
        os[i] = postLp.processSample(s);
    }
    oversampling.processSamplesDown(mono);

    for (int i = 0; i < n; ++i)
    {
        const float lvl = juce::jmap(volSm.getNextValue(), 0.0f, 1.0f, 0.3f, 1.15f);
        filterSm.getNextValue();
        updateFilters();
        p[i] = filterLp.processSample(p[i]) * lvl;
    }
}

void RatDistortion::restoreDefaults()
{
    distortion.store(0.5f);
    filter.store(0.6f);
    volume.store(0.5f);
    setBypassed(true);
}

void RatDistortion::setParameter(const juce::String& id, float value)
{
    value = clamp01(value);
    if (id == "distortion") distortion.store(value);
    else if (id == "filter") filter.store(value);
    else if (id == "volume") volume.store(value);
}

float RatDistortion::getParameter(const juce::String& id) const
{
    if (id == "distortion") return distortion.load();
    if (id == "filter") return filter.load();
    if (id == "volume") return volume.load();
    return 0.0f;
}
