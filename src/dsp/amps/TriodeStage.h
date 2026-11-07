#pragma once

#include "dsp/DspUtils.h"
#include <cmath>

class TriodeStage
{
public:
    void prepare(double sampleRate, float millerHz = 12000.0f, float dcHz = 12.0f)
    {
        sr = (float) sampleRate;
        setMiller(millerHz);
        setDcBlock(dcHz);
        reset();
    }

    void setGain(float linear) noexcept { gain = linear; }
    void setAsymmetry(float posKnee, float negKnee) noexcept
    {
        kPos = juce::jmax(0.05f, posKnee);
        kNeg = juce::jmax(0.05f, negKnee);
    }

    void setMiller(float hz) noexcept
    {
        const float x = std::exp(-2.0f * juce::MathConstants<float>::pi * juce::jlimit(200.0f, sr * 0.45f, hz) / sr);
        lpA = 1.0f - x;
        lpB = x;
    }

    void setDcBlock(float hz) noexcept
    {
        const float x = std::exp(-2.0f * juce::MathConstants<float>::pi * juce::jlimit(2.0f, 80.0f, hz) / sr);
        hpA = x;
    }

    void reset() noexcept
    {
        hpZ = 0.0f;
        lpZ = 0.0f;
        hpX = 0.0f;
    }

    float process(float x) noexcept
    {
        x *= gain;
        float y = x >= 0.0f ? kPos * std::tanh(x / kPos)
                            : kNeg * std::tanh(x / kNeg);
        const float hp = hpA * (hpZ + y - hpX);
        hpX = y;
        hpZ = hp;
        lpZ = lpA * hp + lpB * lpZ;
        return lpZ;
    }

private:
    float sr = 44100.0f;
    float gain = 4.0f;
    float kPos = 0.65f;
    float kNeg = 1.15f;
    float hpA = 0.999f;
    float lpA = 0.2f;
    float lpB = 0.8f;
    float hpZ = 0.0f;
    float hpX = 0.0f;
    float lpZ = 0.0f;
};
