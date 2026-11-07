#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <vector>

namespace dsp_utils
{
inline constexpr int kMaxChannels = 2;

inline float hardClip(float x) noexcept
{
    return juce::jlimit(-1.0f, 1.0f, x);
}

inline float softClip(float x) noexcept
{
    return std::tanh(x);
}

inline float diodeSoft(float x) noexcept
{
    return (float) std::asinh((double) x);
}

inline float diodeHard(float x) noexcept
{
    const float ax = std::abs(x);
    const float n = 2.5f;
    const float d = std::pow(1.0f + std::pow(ax, n), 1.0f / n);
    return (ax < 1.0e-12f) ? 0.0f : x / d;
}

inline float safetyLimit(float x) noexcept
{
    constexpr float threshold = 0.89125094f; // -1 dBFS
    const float ax = std::abs(x);
    if (ax <= threshold)
        return x;

    const float sign = x < 0.0f ? -1.0f : 1.0f;
    const float excess = ax - threshold;
    const float headroom = 1.0f - threshold;
    return sign * (threshold + headroom * std::tanh(excess / headroom));
}

inline juce::dsp::ProcessSpec makeSpec(double sampleRate, int samplesPerBlock) noexcept
{
    return { sampleRate, (juce::uint32) juce::jmax(1, samplesPerBlock), 1 };
}

inline void ensureCoef(juce::dsp::IIR::Filter<float>& filter)
{
    if (filter.coefficients == nullptr)
        filter.coefficients = new juce::dsp::IIR::Coefficients<float>();
}

template <typename CoefArray>
inline void assignCoef(juce::dsp::IIR::Filter<float>& filter, const CoefArray& coef) noexcept
{
    if (filter.coefficients == nullptr)
        return;
    *filter.coefficients = coef;
}

template <size_t N, typename CoefArray>
inline void assignAll(std::array<juce::dsp::IIR::Filter<float>, N>& filters, const CoefArray& coef) noexcept
{
    for (auto& f : filters)
        assignCoef(f, coef);
}

class OnePoleLowpass
{
public:
    void setCutoff(float sampleRate, float hz) noexcept
    {
        const float nyquist = sampleRate * 0.49f;
        const float clamped = juce::jlimit(20.0f, nyquist, hz);
        const float x = std::exp(-2.0f * juce::MathConstants<float>::pi * clamped / sampleRate);
        a0 = 1.0f - x;
        b1 = x;
    }

    float process(float x) noexcept
    {
        z = a0 * x + b1 * z;
        return z;
    }

    void reset() noexcept { z = 0.0f; }

private:
    float a0 = 1.0f;
    float b1 = 0.0f;
    float z = 0.0f;
};

class OnePoleHighpass
{
public:
    void setCutoff(float sampleRate, float hz) noexcept
    {
        const float nyquist = sampleRate * 0.49f;
        const float clamped = juce::jlimit(1.0f, nyquist, hz);
        a = std::exp(-2.0f * juce::MathConstants<float>::pi * clamped / sampleRate);
    }

    float process(float x) noexcept
    {
        const float y = a * (z + x - x1);
        x1 = x;
        z = y;
        return y;
    }

    void reset() noexcept { z = 0.0f; x1 = 0.0f; }

private:
    float a = 0.995f;
    float z = 0.0f;
    float x1 = 0.0f;
};

class DelayLine
{
public:
    void prepare(double sampleRate, float maxDelayMs)
    {
        sr = sampleRate;
        const int n = juce::jmax(4, (int) std::ceil(sampleRate * maxDelayMs * 0.001) + 8);
        buffer.assign((size_t) n, 0.0f);
        writeIndex = 0;
    }

    void reset()
    {
        std::fill(buffer.begin(), buffer.end(), 0.0f);
        writeIndex = 0;
    }

    void write(float x) noexcept
    {
        if (buffer.empty())
            return;

        buffer[(size_t) writeIndex] = x;
        writeIndex = (writeIndex + 1) % (int) buffer.size();
    }

    float readInterpolated(float delaySamples) const noexcept
    {
        if (buffer.empty())
            return 0.0f;

        const int size = (int) buffer.size();
        const float clamped = juce::jlimit(1.0f, (float) size - 2.0f, delaySamples);
        float readPos = (float) writeIndex - clamped;

        while (readPos < 0.0f)
            readPos += (float) size;

        const int i0 = (int) readPos;
        const int i1 = (i0 + 1) % size;
        const float frac = readPos - (float) i0;
        return buffer[(size_t) i0] * (1.0f - frac) + buffer[(size_t) i1] * frac;
    }

    int size() const noexcept { return (int) buffer.size(); }

private:
    std::vector<float> buffer;
    int writeIndex = 0;
    double sr = 44100.0;
};
} // namespace dsp_utils
