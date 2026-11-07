#include "GoldenReverb.h"

std::vector<ParamInfo> GoldenReverb::getParamList() const
{
    return {
        { "decay", "Decay", 0.45f },
        { "predelay", "Predelay", 0.25f },
        { "mix", "Mix", 0.3f },
        { "type", "Type", 0.0f }
    };
}

void GoldenReverb::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;
    for (auto* sm : { &decaySm, &predelaySm, &mixSm })
        sm->reset(sampleRate, 0.05);

    decaySm.setCurrentAndTargetValue(decay.load());
    predelaySm.setCurrentAndTargetValue(predelay.load());
    mixSm.setCurrentAndTargetValue(mix.load());

    const float scale = (float) sampleRate / 44100.0f;
    const int combSizes[8] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
    const int apSizes[4] = { 556, 441, 341, 225 };
    const auto spec = dsp_utils::makeSpec(sampleRate, samplesPerBlock);

    for (int ch = 0; ch < dsp_utils::kMaxChannels; ++ch)
    {
        const int spread = ch * 23;
        for (int i = 0; i < 8; ++i)
            combs[(size_t) ch][(size_t) i].setSize((int) (combSizes[i] * scale) + spread);
        for (int i = 0; i < 4; ++i)
            allpasses[(size_t) ch][(size_t) i].setSize((int) (apSizes[i] * scale) + spread);

        preDelays[(size_t) ch].prepare(sampleRate, 140.0f);
        dripDelays[(size_t) ch].prepare(sampleRate, 90.0f);
        dsp_utils::ensureCoef(wetHp[(size_t) ch]);
        dsp_utils::ensureCoef(wetLp[(size_t) ch]);
        wetHp[(size_t) ch].prepare(spec);
        wetLp[(size_t) ch].prepare(spec);
    }

    const float atkMs = 5.0f;
    const float relMs = 450.0f;
    envAtk = 1.0f - std::exp(-1.0f / (0.001f * atkMs * (float) sampleRate));
    envRel = 1.0f - std::exp(-1.0f / (0.001f * relMs * (float) sampleRate));
    env = 0.0f;
    lastType = -1;
    applyType(typeIndexFrom(type.load()));
}

void GoldenReverb::reset()
{
    env = 0.0f;
    for (int ch = 0; ch < dsp_utils::kMaxChannels; ++ch)
    {
        for (auto& c : combs[(size_t) ch])
            c.reset();
        for (auto& a : allpasses[(size_t) ch])
            a.reset();
        preDelays[(size_t) ch].reset();
        dripDelays[(size_t) ch].reset();
        wetHp[(size_t) ch].reset();
        wetLp[(size_t) ch].reset();
    }
}

void GoldenReverb::applyType(int typeIndex)
{
    if (typeIndex == lastType)
        return;

    const float sr = (float) currentSampleRate;
    float hpHz = 200.0f, lpHz = 6000.0f, apCoeff = 0.5f;
    if (typeIndex == 1)
    {
        hpHz = 140.0f;
        lpHz = 8500.0f;
        apCoeff = 0.55f;
    }
    else if (typeIndex == 2)
    {
        hpHz = 380.0f;
        lpHz = 3400.0f;
        apCoeff = 0.72f;
    }

    const auto hpC = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(sr, hpHz, 0.7f);
    const auto lpC = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(sr, lpHz, 0.7f);
    for (int ch = 0; ch < dsp_utils::kMaxChannels; ++ch)
    {
        dsp_utils::assignCoef(wetHp[(size_t) ch], hpC);
        dsp_utils::assignCoef(wetLp[(size_t) ch], lpC);
        for (auto& a : allpasses[(size_t) ch])
            a.coeff = apCoeff;
    }
    lastType = typeIndex;
}

void GoldenReverb::process(juce::AudioBuffer<float>& buffer)
{
    decaySm.setTargetValue(decay.load());
    predelaySm.setTargetValue(predelay.load());
    mixSm.setTargetValue(mix.load());
    applyType(typeIndexFrom(type.load()));

    const int n = buffer.getNumSamples();
    const int chans = juce::jmin(buffer.getNumChannels(), dsp_utils::kMaxChannels);
    float* ptrs[dsp_utils::kMaxChannels];
    for (int ch = 0; ch < chans; ++ch)
        ptrs[ch] = buffer.getWritePointer(ch);

    const int typeIndex = lastType;
    const int activeCombs = typeIndex == 2 ? 3 : 8;
    const float combNorm = 1.0f / (float) activeCombs;
    const float dripSamples = 58.0f * 0.001f * (float) currentSampleRate;

    for (int i = 0; i < n; ++i)
    {
        const float decayN = decaySm.getNextValue();
        float fb = juce::jmap(decayN, 0.0f, 1.0f, 0.55f, 0.96f);
        float damp = 0.28f;
        if (typeIndex == 1)
        {
            fb = juce::jmap(decayN, 0.0f, 1.0f, 0.62f, 0.97f);
            damp = 0.10f;
        }
        else if (typeIndex == 2)
        {
            fb = juce::jmap(decayN, 0.0f, 1.0f, 0.72f, 0.985f);
            damp = 0.05f;
        }

        const float preMs = juce::jmap(predelaySm.getNextValue(), 0.0f, 1.0f, 0.0f, 120.0f);
        const float mixN = mixSm.getNextValue();
        const float wetGain = 0.7f * std::pow(mixN, 1.6f);
        const float preSamples = juce::jmax(1.0f, preMs * 0.001f * (float) currentSampleRate);

        float dryPeak = 0.0f;
        for (int ch = 0; ch < chans; ++ch)
            dryPeak = juce::jmax(dryPeak, std::abs(ptrs[ch][i]));

        const float coeff = dryPeak > env ? envAtk : envRel;
        env += coeff * (dryPeak - env);
        const float duck = juce::jmap(juce::jlimit(0.0f, 1.0f, env * 2.2f), 0.0f, 1.0f, 1.0f, 0.68f);

        for (int ch = 0; ch < chans; ++ch)
        {
            for (int c = 0; c < activeCombs; ++c)
            {
                combs[(size_t) ch][(size_t) c].feedback = fb;
                combs[(size_t) ch][(size_t) c].damp = damp;
            }

            const float dry = ptrs[ch][i];
            float filtered = wetHp[(size_t) ch].processSample(dry);
            filtered = wetLp[(size_t) ch].processSample(filtered);

            preDelays[(size_t) ch].write(filtered);
            const float pred = preDelays[(size_t) ch].readInterpolated(preSamples);

            float acc = 0.0f;
            for (int c = 0; c < activeCombs; ++c)
                acc += combs[(size_t) ch][(size_t) c].process(pred);
            acc *= combNorm;
            for (auto& a : allpasses[(size_t) ch])
                acc = a.process(acc);

            if (typeIndex == 2)
            {
                dripDelays[(size_t) ch].write(acc);
                acc = 0.62f * acc + 0.38f * dripDelays[(size_t) ch].readInterpolated(dripSamples);
            }

            ptrs[ch][i] = dry + acc * wetGain * duck;
        }
    }
}

void GoldenReverb::restoreDefaults()
{
    decay.store(0.45f);
    predelay.store(0.25f);
    mix.store(0.3f);
    type.store(0.0f);
    setBypassed(true);
}

void GoldenReverb::setParameter(const juce::String& id, float value)
{
    value = clamp01(value);
    if (id == "decay") decay.store(value);
    else if (id == "predelay") predelay.store(value);
    else if (id == "mix") mix.store(value);
    else if (id == "type") type.store(typeValueFromIndex(typeIndexFrom(value)));
}

float GoldenReverb::getParameter(const juce::String& id) const
{
    if (id == "decay") return decay.load();
    if (id == "predelay") return predelay.load();
    if (id == "mix") return mix.load();
    if (id == "type") return type.load();
    return 0.0f;
}
