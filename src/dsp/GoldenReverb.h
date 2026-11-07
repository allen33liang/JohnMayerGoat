#pragma once

#include "Effect.h"
#include "DspUtils.h"

class GoldenReverb : public Effect
{
public:
    juce::String getId() const override { return "reverb"; }
    juce::String getDisplayName() const override { return "Reverb"; }
    juce::Colour getColour() const override { return juce::Colour(0xffc4b08a); }
    std::vector<ParamInfo> getParamList() const override;

    void prepare(double sampleRate, int samplesPerBlock) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void restoreDefaults() override;
    void setParameter(const juce::String& id, float value) override;
    float getParameter(const juce::String& id) const override;

    static int typeIndexFrom(float value) noexcept
    {
        if (value < 0.33f) return 0;
        if (value < 0.66f) return 1;
        return 2;
    }

    static float typeValueFromIndex(int index) noexcept
    {
        return index <= 0 ? 0.0f : (index == 1 ? 0.5f : 1.0f);
    }

private:
    struct Comb
    {
        std::vector<float> buf;
        int idx = 0;
        float filterStore = 0.0f;
        float feedback = 0.84f;
        float damp = 0.2f;

        void setSize(int n)
        {
            buf.assign((size_t) juce::jmax(1, n), 0.0f);
            idx = 0;
            filterStore = 0.0f;
        }

        void reset()
        {
            std::fill(buf.begin(), buf.end(), 0.0f);
            idx = 0;
            filterStore = 0.0f;
        }

        float process(float input) noexcept
        {
            if (buf.empty())
                return 0.0f;
            const float output = buf[(size_t) idx];
            filterStore = output * (1.0f - damp) + filterStore * damp;
            buf[(size_t) idx] = input + filterStore * feedback;
            if (++idx >= (int) buf.size())
                idx = 0;
            return output;
        }
    };

    struct Allpass
    {
        std::vector<float> buf;
        int idx = 0;
        float coeff = 0.5f;

        void setSize(int n)
        {
            buf.assign((size_t) juce::jmax(1, n), 0.0f);
            idx = 0;
        }

        void reset()
        {
            std::fill(buf.begin(), buf.end(), 0.0f);
            idx = 0;
        }

        float process(float input) noexcept
        {
            if (buf.empty())
                return 0.0f;
            const float bufout = buf[(size_t) idx];
            const float output = -input + bufout;
            buf[(size_t) idx] = input + bufout * coeff;
            if (++idx >= (int) buf.size())
                idx = 0;
            return output;
        }
    };

    void applyType(int typeIndex);

    std::atomic<float> decay { 0.45f };
    std::atomic<float> predelay { 0.25f };
    std::atomic<float> mix { 0.3f };
    std::atomic<float> type { 0.0f };

    juce::SmoothedValue<float> decaySm, predelaySm, mixSm;
    std::array<std::array<Comb, 8>, dsp_utils::kMaxChannels> combs;
    std::array<std::array<Allpass, 4>, dsp_utils::kMaxChannels> allpasses;
    std::array<dsp_utils::DelayLine, dsp_utils::kMaxChannels> preDelays, dripDelays;
    std::array<juce::dsp::IIR::Filter<float>, dsp_utils::kMaxChannels> wetHp, wetLp;
    float env = 0.0f;
    float envAtk = 0.0f;
    float envRel = 0.0f;
    int lastType = -1;
};
