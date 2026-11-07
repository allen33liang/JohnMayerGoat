#include "SlapbackDelay.h"

std::vector<ParamInfo> SlapbackDelay::getParamList() const
{
    return {
        { "delay", "Delay", 0.32f },
        { "feedback", "Feedback", 0.22f },
        { "blend", "Blend", 0.4f }
    };
}

void SlapbackDelay::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;
    delaySm.reset(sampleRate, 0.08);
    fbSm.reset(sampleRate, 0.03);
    blendSm.reset(sampleRate, 0.03);
    delaySm.setCurrentAndTargetValue(delay.load());
    fbSm.setCurrentAndTargetValue(feedback.load());
    blendSm.setCurrentAndTargetValue(blend.load());

    for (int i = 0; i < dsp_utils::kMaxChannels; ++i)
    {
        delays[(size_t) i].prepare(sampleRate, 400.0f);
        bbdLp[(size_t) i].setCutoff((float) sampleRate, 3800.0f);
    }
    reset();
}

void SlapbackDelay::reset()
{
    for (int i = 0; i < dsp_utils::kMaxChannels; ++i)
    {
        delays[(size_t) i].reset();
        bbdLp[(size_t) i].reset();
    }
}

void SlapbackDelay::process(juce::AudioBuffer<float>& buffer)
{
    delaySm.setTargetValue(delay.load());
    fbSm.setTargetValue(feedback.load());
    blendSm.setTargetValue(blend.load());

    const int n = buffer.getNumSamples();
    const int chans = juce::jmin(buffer.getNumChannels(), dsp_utils::kMaxChannels);
    float* ptrs[dsp_utils::kMaxChannels];
    for (int ch = 0; ch < chans; ++ch)
        ptrs[ch] = buffer.getWritePointer(ch);

    for (int i = 0; i < n; ++i)
    {
        const float delayNorm = delaySm.getNextValue();
        const float delayMs = juce::jmap(delayNorm, 0.0f, 1.0f, 25.0f, 280.0f);
        const float fb = juce::jmap(fbSm.getNextValue(), 0.0f, 1.0f, 0.0f, 0.62f);
        const float mx = blendSm.getNextValue();
        const float cutoff = juce::jmap(delayNorm, 0.0f, 1.0f, 5200.0f, 2100.0f);
        const float delaySamples = delayMs * 0.001f * (float) currentSampleRate;

        for (int ch = 0; ch < chans; ++ch)
        {
            bbdLp[(size_t) ch].setCutoff((float) currentSampleRate, cutoff);
            const float dry = ptrs[ch][i];
            float delayed = delays[(size_t) ch].readInterpolated(delaySamples);
            delayed = bbdLp[(size_t) ch].process(delayed);
            delayed = dsp_utils::softClip(delayed * 1.08f);
            delays[(size_t) ch].write(dry + delayed * fb);
            ptrs[ch][i] = dry * (1.0f - mx) + delayed * mx;
        }
    }
}

void SlapbackDelay::restoreDefaults()
{
    delay.store(0.32f);
    feedback.store(0.22f);
    blend.store(0.4f);
    setBypassed(true);
}

void SlapbackDelay::setParameter(const juce::String& id, float value)
{
    value = clamp01(value);
    if (id == "delay") delay.store(value);
    else if (id == "feedback") feedback.store(value);
    else if (id == "blend") blend.store(value);
}

float SlapbackDelay::getParameter(const juce::String& id) const
{
    if (id == "delay") return delay.load();
    if (id == "feedback") return feedback.load();
    if (id == "blend") return blend.load();
    return 0.0f;
}
