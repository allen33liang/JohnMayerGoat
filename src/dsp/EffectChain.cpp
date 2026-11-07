#include "DigitalPreamp.h"
#include "TubeScreamer.h"
#include "Bluesbreaker.h"
#include "RatDistortion.h"
#include "Shredmaster.h"
#include "Chorus.h"
#include "TapeDelay.h"
#include "SlapbackDelay.h"
#include "GoldenReverb.h"
#include "CabinetSim.h"

#include "EffectChain.h"
#include <algorithm>

namespace
{
std::shared_ptr<Effect> cloneById(const juce::String& id)
{
    if (id == "tubeScreamer") return std::make_shared<TubeScreamer>();
    if (id == "bluesbreaker") return std::make_shared<Bluesbreaker>();
    if (id == "rat") return std::make_shared<RatDistortion>();
    if (id == "shredmaster") return std::make_shared<Shredmaster>();
    if (id == "preamp") return std::make_shared<DigitalPreamp>();
    if (id == "chorus") return std::make_shared<Chorus>();
    if (id == "delay") return std::make_shared<TapeDelay>();
    if (id == "slapback") return std::make_shared<SlapbackDelay>();
    if (id == "reverb") return std::make_shared<GoldenReverb>();
    return {};
}
}

juce::StringArray EffectChain::defaultOrder()
{
    return { "tubeScreamer", "bluesbreaker", "rat", "shredmaster",
             "preamp", "chorus", "delay", "slapback", "reverb" };
}

EffectChain::EffectChain()
{
    for (const auto& id : defaultOrder())
        registry.push_back(makeEffect(id));
    rebuildList(defaultOrder());
}

std::shared_ptr<Effect> EffectChain::makeEffect(const juce::String& id)
{
    return cloneById(id);
}

void EffectChain::prepare(double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    blockSize = samplesPerBlock;
    for (auto& fx : registry)
        fx->setPrepared(sampleRate, samplesPerBlock);
}

void EffectChain::reset()
{
    for (auto& fx : registry)
        fx->reset();
}

void EffectChain::process(juce::AudioBuffer<float>& buffer, CabinetSim* cabinet)
{
    auto node = std::atomic_load(&liveHead);
    bool stereo = false;
    bool cabDone = false;

    auto runCabAndSplit = [&] ()
    {
        if (! cabDone && cabinet != nullptr)
        {
            cabinet->process(buffer);
            cabDone = true;
        }
        if (! stereo && buffer.getNumChannels() > 1)
        {
            buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());
            stereo = true;
        }
    };

    while (node != nullptr)
    {
        if (node->effect != nullptr)
        {
            if (Effect::isTimeBasedId(node->effect->getId()))
                runCabAndSplit();

            node->effect->processWithFade(buffer);
        }
        node = node->next;
    }

    if (! cabDone && cabinet != nullptr)
        cabinet->process(buffer);
    if (! stereo && buffer.getNumChannels() > 1)
        buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());
}

std::shared_ptr<EffectNode> EffectChain::getHead() const
{
    return std::atomic_load(&liveHead);
}

std::vector<std::shared_ptr<Effect>> EffectChain::getOrderedEffects() const
{
    std::vector<std::shared_ptr<Effect>> out;
    auto node = std::atomic_load(&liveHead);
    while (node != nullptr)
    {
        out.push_back(node->effect);
        node = node->next;
    }
    return out;
}

std::shared_ptr<Effect> EffectChain::findById(const juce::String& id) const
{
    for (auto& fx : registry)
        if (fx->getId() == id)
            return fx;
    return {};
}

juce::StringArray EffectChain::getOrderIds() const
{
    juce::StringArray ids;
    for (auto& fx : getOrderedEffects())
        ids.add(fx->getId());
    return ids;
}

void EffectChain::rebuildList(const juce::StringArray& ids)
{
    std::shared_ptr<EffectNode> head, tail;
    juce::StringArray used;

    auto append = [&] (const std::shared_ptr<Effect>& fx)
    {
        if (fx == nullptr || used.contains(fx->getId()))
            return;
        auto node = std::make_shared<EffectNode>();
        node->effect = fx;
        if (head == nullptr)
            head = node;
        else
            tail->next = node;
        tail = node;
        used.add(fx->getId());
    };

    for (const auto& id : ids)
        append(findById(id));

    for (auto& fx : registry)
        append(fx);

    std::atomic_store(&liveHead, head);
}

void EffectChain::setOrder(const juce::StringArray& ids)
{
    rebuildList(ids);
}

void EffectChain::moveEffect(int fromIndex, int toIndex)
{
    auto ids = getOrderIds();
    if (! juce::isPositiveAndBelow(fromIndex, ids.size()))
        return;

    toIndex = juce::jlimit(0, ids.size() - 1, toIndex);
    const auto id = ids[fromIndex];
    ids.remove(fromIndex);
    ids.insert(toIndex, id);
    rebuildList(ids);
}

void EffectChain::restoreDefaultOrder()
{
    rebuildList(defaultOrder());
}

juce::var EffectChain::toJson() const
{
    juce::Array<juce::var> items;
    for (auto& fx : getOrderedEffects())
        items.add(fx->toJson());
    return items;
}

void EffectChain::fromJson(const juce::var& json)
{
    if (! json.isArray())
        return;

    juce::StringArray order;
    if (auto* arr = json.getArray())
    {
        for (const auto& item : *arr)
        {
            if (auto* obj = item.getDynamicObject())
            {
                const auto id = obj->getProperty("id").toString();
                order.add(id);
                if (auto fx = findById(id))
                    fx->fromJson(item);
            }
        }
    }
    rebuildList(order);
}

juce::String EffectChain::slotLabelFor(const juce::String& effectId) const
{
    bool seenPreamp = false;
    for (auto& fx : getOrderedEffects())
    {
        if (fx->getId() == "preamp")
        {
            if (effectId == "preamp")
                return "AMP";
            seenPreamp = true;
            continue;
        }
        if (fx->getId() == effectId)
            return seenPreamp ? "POST" : "PRE";
    }
    return {};
}
