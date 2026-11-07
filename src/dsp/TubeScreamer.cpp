#include "TubeScreamer.h"

std::vector<ParamInfo> TubeScreamer::getParamList() const
{
    return { { "drive", "Drive", 0.4f }, { "tone", "Tone", 0.55f }, { "level", "Level", 0.5f } };
}

void TubeScreamer::prepare(double sampleRate, int samplesPerBlock)
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
    const float osRate = (float) sampleRate * 4.0f;
    const auto osSpec = dsp_utils::makeSpec((double) osRate, samplesPerBlock * 4);
    const auto baseSpec = dsp_utils::makeSpec(sampleRate, samplesPerBlock);

    for (auto* f : { &clipHp, &preLp, &fbLp, &postLp, &toneLp, &toneHs })
        dsp_utils::ensureCoef(*f);
    clipHp.prepare(osSpec);
    preLp.prepare(osSpec);
    fbLp.prepare(osSpec);
    postLp.prepare(osSpec);
    toneLp.prepare(baseSpec);
    toneHs.prepare(baseSpec);
    dcBlock.setCutoff(osRate, 12.0f);

    dsp_utils::assignCoef(clipHp, juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderHighPass(osRate, 720.0f));
    dsp_utils::assignCoef(preLp, juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(osRate, 3800.0f, 0.7f));
    dsp_utils::assignCoef(postLp, juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(osRate, 3500.0f, 0.7f));
    dsp_utils::assignCoef(toneLp, juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderLowPass((float) sampleRate, 720.0f));
    lastTone = lastDrive = -1.0f;
    updateFilters();
    reset();
}

void TubeScreamer::reset()
{
    oversampling.reset();
    clipHp.reset();
    preLp.reset();
    fbLp.reset();
    postLp.reset();
    toneLp.reset();
    toneHs.reset();
    dcBlock.reset();
}

void TubeScreamer::updateFilters()
{
    const float toneN = toneSm.getCurrentValue();
    const float driveN = driveSm.getCurrentValue();
    const float osRate = (float) currentSampleRate * 4.0f;
    const float sr = (float) currentSampleRate;

    if (std::abs(driveN - lastDrive) > 1.0e-4f)
    {
        const float fbHz = juce::jmap(driveN, 0.0f, 1.0f, 5500.0f, 2800.0f);
        dsp_utils::assignCoef(fbLp, juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(osRate, fbHz, 0.55f));
        lastDrive = driveN;
    }
    if (std::abs(toneN - lastTone) > 1.0e-4f)
    {
        const float tGain = juce::Decibels::decibelsToGain(juce::jmap(toneN, 0.0f, 1.0f, -6.0f, 5.0f));
        dsp_utils::assignCoef(toneHs, juce::dsp::IIR::ArrayCoefficients<float>::makeHighShelf(sr, 3200.0f, 0.7f, tGain));
        lastTone = toneN;
    }
}

void TubeScreamer::process(juce::AudioBuffer<float>& buffer)
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

    float d = juce::Decibels::decibelsToGain(juce::jmap(driveSm.getCurrentValue(), 0.0f, 1.0f, 0.0f, 28.0f));
    for (int i = 0; i < osN; ++i)
    {
        if ((i % stride) == 0)
        {
            d = juce::Decibels::decibelsToGain(juce::jmap(driveSm.getNextValue(), 0.0f, 1.0f, 0.0f, 28.0f));
            updateFilters();
        }
        const float dry = os[i];
        float g = clipHp.processSample(dry);
        g = preLp.processSample(g);
        g = dsp_utils::softClip(g * d * 1.1f);
        g = fbLp.processSample(g);
        g = dcBlock.process(g);
        os[i] = postLp.processSample(dry + g * 0.78f);
    }
    oversampling.processSamplesDown(mono);

    for (int i = 0; i < n; ++i)
    {
        const float lvl = juce::jmap(levelSm.getNextValue(), 0.0f, 1.0f, 0.3f, 1.1f);
        toneSm.getNextValue();
        updateFilters();
        const float bass = toneLp.processSample(p[i]);
        const float treble = toneHs.processSample(p[i]);
        const float mix = juce::jmap(toneSm.getCurrentValue(), 0.0f, 1.0f, 0.35f, 0.65f);
        p[i] = (bass * (1.0f - mix) + treble * mix) * lvl;
    }
}

void TubeScreamer::restoreDefaults()
{
    drive.store(0.4f);
    tone.store(0.55f);
    level.store(0.5f);
    setBypassed(true);
}

void TubeScreamer::setParameter(const juce::String& id, float value)
{
    value = clamp01(value);
    if (id == "drive") drive.store(value);
    else if (id == "tone") tone.store(value);
    else if (id == "level") level.store(value);
}

float TubeScreamer::getParameter(const juce::String& id) const
{
    if (id == "drive") return drive.load();
    if (id == "tone") return tone.load();
    if (id == "level") return level.load();
    return 0.0f;
}
