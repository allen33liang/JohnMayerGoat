#include "DeluxeReverbModel.h"

std::vector<ParamInfo> DeluxeReverbModel::getParamList() const
{
    return {
        { "volume", "Volume", 0.45f },
        { "treble", "Treble", 0.62f },
        { "bass", "Bass", 0.48f },
        { "bright", "Bright", 0.0f, true }
    };
}

void DeluxeReverbModel::prepare(double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    block = samplesPerBlock;
    volSm.reset(sampleRate, 0.03);
    trebleSm.reset(sampleRate, 0.03);
    bassSm.reset(sampleRate, 0.03);
    volSm.setCurrentAndTargetValue(volume.load());
    trebleSm.setCurrentAndTargetValue(treble.load());
    bassSm.setCurrentAndTargetValue(bass.load());

    v1.prepare(sampleRate, 12000.0f, 6.0f);
    v2.prepare(sampleRate, 14000.0f, 12.0f);
    v1.setAsymmetry(0.68f, 1.12f);
    v2.setAsymmetry(0.75f, 1.08f);
    stack.setParts(ToneStack::deluxeAb763());
    stack.setTopology(ToneStack::Topology::Fender);
    stack.prepare(sampleRate);
    power.prepare(sampleRate);
    power.enablePresence(false);
    power.setSag(0.38f, 10.0f, 220.0f);
    const float hpHz = 3500.0f;
    const float x = std::exp(-2.0f * juce::MathConstants<float>::pi * hpHz / (float) sampleRate);
    brightHpA = x;
    lastT = lastB = lastV = lastBright = -1.0f;
    reset();
}

void DeluxeReverbModel::reset()
{
    v1.reset();
    v2.reset();
    stack.reset();
    power.reset();
    brightZ = brightX = 0.0f;
}

void DeluxeReverbModel::updateControls()
{
    const float vol = volSm.getCurrentValue();
    const float t = trebleSm.getCurrentValue();
    const float b = bassSm.getCurrentValue();
    const float br = bright.load() >= 0.5f ? 1.0f : 0.0f;
    if (std::abs(t - lastT) > 1.0e-4f || std::abs(b - lastB) > 1.0e-4f)
    {
        stack.setTones(t, 0.42f, b);
        lastT = t;
        lastB = b;
    }
    if (std::abs(vol - lastV) > 1.0e-4f)
    {
        const float pre = juce::jmap(vol, 0.0f, 1.0f, 0.9f, 5.8f);
        v1.setGain(pre);
        v2.setGain(juce::jmap(vol, 0.0f, 1.0f, 0.85f, 3.2f));
        power.setDrive(juce::jmap(vol, 0.0f, 1.0f, 0.7f, 1.85f));
        lastV = vol;
    }
    lastBright = br;
}

void DeluxeReverbModel::process(float* samples, int numSamples)
{
    volSm.setTargetValue(volume.load());
    trebleSm.setTargetValue(treble.load());
    bassSm.setTargetValue(bass.load());

    for (int i = 0; i < numSamples; ++i)
    {
        volSm.getNextValue();
        trebleSm.getNextValue();
        bassSm.getNextValue();
        updateControls();

        float s = samples[i];
        s = v1.process(s);
        s = stack.process(s);
        const float vol = volSm.getCurrentValue();
        if (bright.load() >= 0.5f)
        {
            const float hp = brightHpA * (brightZ + s - brightX);
            brightX = s;
            brightZ = hp;
            s += hp * juce::jmap(vol, 0.0f, 1.0f, 0.55f, 0.12f);
        }
        s = v2.process(s);
        s = power.process(s);
        samples[i] = s * 0.72f;
    }
}

void DeluxeReverbModel::restoreDefaults()
{
    volume.store(0.45f);
    treble.store(0.62f);
    bass.store(0.48f);
    bright.store(0.0f);
}

void DeluxeReverbModel::setParameter(const juce::String& id, float value)
{
    value = clamp01(value);
    if (id == "volume" || id == "gain") volume.store(value);
    else if (id == "treble") treble.store(value);
    else if (id == "bass") bass.store(value);
    else if (id == "bright") bright.store(value >= 0.5f ? 1.0f : 0.0f);
}

float DeluxeReverbModel::getParameter(const juce::String& id) const
{
    if (id == "volume" || id == "gain") return volume.load();
    if (id == "treble") return treble.load();
    if (id == "bass") return bass.load();
    if (id == "bright") return bright.load();
    return 0.0f;
}
