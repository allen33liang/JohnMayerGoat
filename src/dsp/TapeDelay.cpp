#include "TapeDelay.h"

std::vector<ParamInfo> TapeDelay::getParamList() const
{
    return { { "time", "Delay", 0.35f }, { "feedback", "Repeats", 0.4f }, { "mix", "Volume", 0.35f } };
}

void TapeDelay::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;
    timeSm.reset(sampleRate, 0.08);
    fbSm.reset(sampleRate, 0.03);
    mixSm.reset(sampleRate, 0.03);
    timeSm.setCurrentAndTargetValue(time.load());
    fbSm.setCurrentAndTargetValue(feedback.load());
    mixSm.setCurrentAndTargetValue(mix.load());

    for (int i = 0; i < dsp_utils::kMaxChannels; ++i)
    {
        delays[(size_t) i].prepare(sampleRate, 2000.0f);
        tapeLp[(size_t) i].setCutoff((float) sampleRate, 3200.0f);
    }
    reset();
}

void TapeDelay::reset()
{
    for (int i = 0; i < dsp_utils::kMaxChannels; ++i)
    {
        delays[(size_t) i].reset();
        tapeLp[(size_t) i].reset();
    }
}

void TapeDelay::process(juce::AudioBuffer<float>& buffer)
{
    timeSm.setTargetValue(time.load());
    fbSm.setTargetValue(feedback.load());
    mixSm.setTargetValue(mix.load());

    const int n = buffer.getNumSamples();
    const int chans = juce::jmin(buffer.getNumChannels(), dsp_utils::kMaxChannels);
    float* ptrs[dsp_utils::kMaxChannels];
    for (int ch = 0; ch < chans; ++ch)
        ptrs[ch] = buffer.getWritePointer(ch);

    for (int i = 0; i < n; ++i)
    {
        const float delayMs = juce::jmap(timeSm.getNextValue(), 0.0f, 1.0f, 80.0f, 650.0f);
        const float fb = juce::jmap(fbSm.getNextValue(), 0.0f, 1.0f, 0.05f, 0.82f);
        const float mx = mixSm.getNextValue();
        const float delaySamples = delayMs * 0.001f * (float) currentSampleRate;

        for (int ch = 0; ch < chans; ++ch)
        {
            const float dry = ptrs[ch][i];
            float delayed = delays[(size_t) ch].readInterpolated(delaySamples);
            delayed = tapeLp[(size_t) ch].process(delayed);
            delayed = dsp_utils::softClip(delayed * 0.95f);
            delays[(size_t) ch].write(dry + delayed * fb);
            ptrs[ch][i] = dry * (1.0f - mx) + delayed * mx * 0.72f;
        }
    }
}

void TapeDelay::restoreDefaults()
{
    time.store(0.35f);
    feedback.store(0.4f);
    mix.store(0.35f);
    setBypassed(true);
}

void TapeDelay::setParameter(const juce::String& id, float value)
{
    value = clamp01(value);
    if (id == "time") time.store(value);
    else if (id == "feedback") feedback.store(value);
    else if (id == "mix") mix.store(value);
}

float TapeDelay::getParameter(const juce::String& id) const
{
    if (id == "time") return time.load();
    if (id == "feedback") return feedback.load();
    if (id == "mix") return mix.load();
    return 0.0f;
}
