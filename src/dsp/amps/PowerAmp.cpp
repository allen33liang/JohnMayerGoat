#include "PowerAmp.h"

void PowerAmp::prepare(double sampleRate)
{
    sr = (float) juce::jmax(1000.0, sampleRate);
    setSag(sagAmt, 8.0f, 180.0f);
    presDirty = true;
    reset();
}

void PowerAmp::reset()
{
    env = 0.0f;
    hpZ = hpX = lpZ = 0.0f;
}

void PowerAmp::setSag(float amount, float attackMs, float releaseMs) noexcept
{
    sagAmt = juce::jlimit(0.0f, 1.2f, amount);
    atk = 1.0f - std::exp(-1.0f / (0.001f * juce::jmax(1.0f, attackMs) * sr));
    rel = 1.0f - std::exp(-1.0f / (0.001f * juce::jmax(20.0f, releaseMs) * sr));
}

void PowerAmp::setPresence(float zeroToOne) noexcept
{
    presenceN = juce::jlimit(0.0f, 1.0f, zeroToOne);
    presDirty = true;
}

void PowerAmp::setResonance(float zeroToOne) noexcept
{
    resonanceN = juce::jlimit(0.0f, 1.0f, zeroToOne);
    presDirty = true;
}

void PowerAmp::updatePresence()
{
    const float hpHz = juce::jmap(presenceN, 0.0f, 1.0f, 2500.0f, 6500.0f);
    const float lpHz = juce::jmap(resonanceN, 0.0f, 1.0f, 80.0f, 220.0f);
    const float hpx = std::exp(-2.0f * juce::MathConstants<float>::pi * hpHz / sr);
    hpA = hpx;
    const float lpx = std::exp(-2.0f * juce::MathConstants<float>::pi * lpHz / sr);
    lpA = 1.0f - lpx;
    presDirty = false;
}

float PowerAmp::process(float x) noexcept
{
    const float ax = std::abs(x);
    env += (ax > env ? atk : rel) * (ax - env);
    const float sag = 1.0f / (1.0f + sagAmt * env * 2.4f);
    float y = std::tanh(x * drive * sag);
    y = 1.5f * y - 0.5f * y * y * y;

    if (usePresence)
    {
        if (presDirty)
            updatePresence();
        const float hp = hpA * (hpZ + y - hpX);
        hpX = y;
        hpZ = hp;
        lpZ += lpA * (y - lpZ);
        const float presMix = juce::jmap(presenceN, 0.0f, 1.0f, -0.25f, 0.45f);
        const float resMix = juce::jmap(resonanceN, 0.0f, 1.0f, 0.0f, 0.22f);
        y += hp * presMix + lpZ * resMix;
    }
    return y;
}
