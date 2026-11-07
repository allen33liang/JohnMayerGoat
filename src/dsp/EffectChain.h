#pragma once

#include "Effect.h"
#include <memory>
#include <vector>

class CabinetSim;

struct EffectNode
{
    std::shared_ptr<Effect> effect;
    std::shared_ptr<EffectNode> next;
};

class EffectChain
{
public:
    EffectChain();

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    void process(juce::AudioBuffer<float>& buffer, CabinetSim* cabinet = nullptr);

    std::shared_ptr<EffectNode> getHead() const;
    std::vector<std::shared_ptr<Effect>> getOrderedEffects() const;
    std::shared_ptr<Effect> findById(const juce::String& id) const;
    juce::StringArray getOrderIds() const;

    void setOrder(const juce::StringArray& ids);
    void moveEffect(int fromIndex, int toIndex);
    void restoreDefaultOrder();

    juce::var toJson() const;
    void fromJson(const juce::var& json);

    static juce::StringArray defaultOrder();

    juce::String slotLabelFor(const juce::String& effectId) const;

private:
    void rebuildList(const juce::StringArray& ids);
    std::shared_ptr<Effect> makeEffect(const juce::String& id);

    std::vector<std::shared_ptr<Effect>> registry;
    std::shared_ptr<EffectNode> liveHead;
    double sr = 44100.0;
    int blockSize = 512;
};
