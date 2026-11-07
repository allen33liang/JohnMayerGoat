#include "SteelStringModel.h"

std::vector<ParamInfo> SteelStringModel::getParamList() const
{
    return {
        { "volume", "Volume", 0.5f },
        { "treble", "Treble", 0.55f },
        { "middle", "Middle", 0.58f },
        { "bass", "Bass", 0.52f },
        { "bright", "Bright", 0.0f, true }
    };
}

void SteelStringModel::prepare(double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    block = samplesPerBlock;
    for (auto* sm : { &volSm, &trebleSm, &midSm, &bassSm })
        sm->reset(sampleRate, 0.03);
    volSm.setCurrentAndTargetValue(volume.load());
    trebleSm.setCurrentAndTargetValue(treble.load());
    midSm.setCurrentAndTargetValue(middle.load());
    bassSm.setCurrentAndTargetValue(bass.load());

    v1.prepare(sampleRate, 16000.0f, 5.0f);
    v2.prepare(sampleRate, 15000.0f, 8.0f);
    v3.prepare(sampleRate, 14000.0f, 10.0f);
    v1.setAsymmetry(0.88f, 1.04f);
    v2.setAsymmetry(0.9f, 1.03f);
    v3.setAsymmetry(0.92f, 1.02f);
    stack.setParts(ToneStack::steelString());
    stack.setTopology(ToneStack::Topology::Fender);
    stack.prepare(sampleRate);
    power.prepare(sampleRate);
    power.enablePresence(true);
    power.setSag(0.08f, 12.0f, 280.0f);
    power.setPresence(0.42f);
    power.setResonance(0.4f);
    brightA = std::exp(-2.0f * juce::MathConstants<float>::pi * 2800.0f / (float) sampleRate);
    lastT = lastM = lastB = lastV = -1.0f;
    reset();
}

void SteelStringModel::reset()
{
    v1.reset();
    v2.reset();
    v3.reset();
    stack.reset();
    power.reset();
    brightZ = brightX = 0.0f;
}

void SteelStringModel::updateControls()
{
    const float vol = volSm.getCurrentValue();
    const float t = trebleSm.getCurrentValue();
    const float m = midSm.getCurrentValue();
    const float b = bassSm.getCurrentValue();
    if (std::abs(t - lastT) > 1.0e-4f || std::abs(m - lastM) > 1.0e-4f || std::abs(b - lastB) > 1.0e-4f)
    {
        stack.setTones(t, m, b);
        lastT = t;
        lastM = m;
        lastB = b;
    }
    if (std::abs(vol - lastV) > 1.0e-4f)
    {
        v1.setGain(juce::jmap(vol, 0.0f, 1.0f, 0.85f, 2.6f));
        v2.setGain(juce::jmap(vol, 0.0f, 1.0f, 0.8f, 1.9f));
        v3.setGain(1.05f);
        power.setDrive(juce::jmap(vol, 0.0f, 1.0f, 0.65f, 1.15f));
        lastV = vol;
    }
}

void SteelStringModel::process(float* samples, int numSamples)
{
    volSm.setTargetValue(volume.load());
    trebleSm.setTargetValue(treble.load());
    midSm.setTargetValue(middle.load());
    bassSm.setTargetValue(bass.load());

    const bool br = bright.load() >= 0.5f;

    for (int i = 0; i < numSamples; ++i)
    {
        volSm.getNextValue();
        trebleSm.getNextValue();
        midSm.getNextValue();
        bassSm.getNextValue();
        updateControls();

        float s = samples[i];
        s = v1.process(s);
        s = stack.process(s);
        if (br)
        {
            const float hp = brightA * (brightZ + s - brightX);
            brightX = s;
            brightZ = hp;
            s += hp * 0.28f;
        }
        s = v2.process(s);
        s = v3.process(s);
        s = power.process(s);
        samples[i] = s * 0.68f;
    }
}

void SteelStringModel::restoreDefaults()
{
    volume.store(0.5f);
    treble.store(0.55f);
    middle.store(0.58f);
    bass.store(0.52f);
    bright.store(0.0f);
}

void SteelStringModel::setParameter(const juce::String& id, float value)
{
    value = clamp01(value);
    if (id == "volume" || id == "gain") volume.store(value);
    else if (id == "treble") treble.store(value);
    else if (id == "middle" || id == "mid") middle.store(value);
    else if (id == "bass") bass.store(value);
    else if (id == "bright") bright.store(value >= 0.5f ? 1.0f : 0.0f);
}

float SteelStringModel::getParameter(const juce::String& id) const
{
    if (id == "volume" || id == "gain") return volume.load();
    if (id == "treble") return treble.load();
    if (id == "middle" || id == "mid") return middle.load();
    if (id == "bass") return bass.load();
    if (id == "bright") return bright.load();
    return 0.0f;
}
