#include "Chorus.h"

std::vector<ParamInfo> Chorus::getParamList() const
{
    return { { "rate", "Rate", 0.35f }, { "depth", "Depth", 0.45f }, { "mix", "Mix", 0.5f } };
}

void Chorus::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;
    rateSm.reset(sampleRate, 0.03);
    depthSm.reset(sampleRate, 0.03);
    mixSm.reset(sampleRate, 0.03);
    rateSm.setCurrentAndTargetValue(rate.load());
    depthSm.setCurrentAndTargetValue(depth.load());
    mixSm.setCurrentAndTargetValue(mix.load());

    for (int i = 0; i < dsp_utils::kMaxChannels; ++i)
        delays[(size_t) i].prepare(sampleRate, 50.0f);
    reset();
}

void Chorus::reset()
{
    lfoPhase = 0.0f;
    for (int i = 0; i < dsp_utils::kMaxChannels; ++i)
        delays[(size_t) i].reset();
}

void Chorus::process(juce::AudioBuffer<float>& buffer)
{
    rateSm.setTargetValue(rate.load());
    depthSm.setTargetValue(depth.load());
    mixSm.setTargetValue(mix.load());

    const int n = buffer.getNumSamples();
    const int chans = juce::jmin(buffer.getNumChannels(), dsp_utils::kMaxChannels);
    const float twoPi = juce::MathConstants<float>::twoPi;
    float* ptrs[dsp_utils::kMaxChannels];
    for (int ch = 0; ch < chans; ++ch)
        ptrs[ch] = buffer.getWritePointer(ch);

    for (int i = 0; i < n; ++i)
    {
        const float rateHz = juce::jmap(rateSm.getNextValue(), 0.0f, 1.0f, 0.12f, 2.4f);
        const float dep = depthSm.getNextValue();
        const float mx = mixSm.getNextValue();

        lfoPhase += twoPi * rateHz / (float) currentSampleRate;
        if (lfoPhase > twoPi)
            lfoPhase -= twoPi;

        for (int ch = 0; ch < chans; ++ch)
        {
            const float phase = lfoPhase + (ch == 0 ? 0.0f : juce::MathConstants<float>::pi);
            const float lfo = 0.5f + 0.5f * std::sin(phase);
            const float delayMs = juce::jmap(lfo * dep, 0.0f, 1.0f, 6.5f, 11.5f);
            const float delaySamples = delayMs * 0.001f * (float) currentSampleRate;

            const float dry = ptrs[ch][i];
            delays[(size_t) ch].write(dry);
            const float wet = delays[(size_t) ch].readInterpolated(delaySamples);
            ptrs[ch][i] = dry + wet * mx * 0.85f;
        }
    }
}

void Chorus::restoreDefaults()
{
    rate.store(0.35f);
    depth.store(0.45f);
    mix.store(0.5f);
    setBypassed(true);
}

void Chorus::setParameter(const juce::String& id, float value)
{
    value = clamp01(value);
    if (id == "rate") rate.store(value);
    else if (id == "depth") depth.store(value);
    else if (id == "mix") mix.store(value);
}

float Chorus::getParameter(const juce::String& id) const
{
    if (id == "rate") return rate.load();
    if (id == "depth") return depth.load();
    if (id == "mix") return mix.load();
    return 0.0f;
}
