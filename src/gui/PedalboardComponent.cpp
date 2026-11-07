#include "PedalboardComponent.h"
#include "GraphicUtils.h"
#include "HardwareTheme.h"
#include <limits>

void PedalboardComponent::AmpMarker::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(2.0f);
    auto caption = b.removeFromBottom(24.0f);
    gfx::paintAmpIcon(g, b);
    g.setColour(juce::Colour(0xfff3e6c4));
    g.setFont(gfx::fontHelveticaBold(18.0f));
    g.drawText("AMP", caption, juce::Justification::centred);
}

PedalboardComponent::RackIconButton::RackIconButton(const juce::String& effectId, const juce::String& name)
    : juce::Button(name), id(effectId), label(name)
{
}

void PedalboardComponent::RackIconButton::paintButton(juce::Graphics& g, bool highlighted, bool)
{
    auto r = getLocalBounds().toFloat().reduced(highlighted ? 1.0f : 2.0f);
    gfx::paintPedalIcon(g, r.removeFromTop(r.getHeight() - 14.0f), id);
    g.setColour(juce::Colour(0xfff3e6c4).withAlpha(0.9f));
    g.setFont(id == "slapback" ? gfx::fontHelveticaBold(9.0f) : gfx::fontCondensed(9.0f));
    g.drawFittedText(label, r.toNearestInt(), juce::Justification::centred, 1);
}

PedalboardComponent::PedalboardComponent(AudioEngine& engineToUse) : engine(engineToUse)
{
    viewport.setViewedComponent(&content, false);
    viewport.setScrollBarsShown(false, true);
    viewport.setScrollBarThickness(8);
    addAndMakeVisible(viewport);
    content.addAndMakeVisible(ampMarker);

    rackLabel.setColour(juce::Label::textColourId, juce::Colour(0xfff3e6c4).withAlpha(0.7f));
    rackLabel.setFont(gfx::fontCondensed(12.0f));
    rackLabel.setJustificationType(juce::Justification::centredLeft);
    rackLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(rackLabel);

    rebuild();
}

juce::Rectangle<int> PedalboardComponent::boardArea() const
{
    auto area = getLocalBounds().reduced(12, 10);
    area.removeFromBottom(78);
    return area;
}

juce::Rectangle<int> PedalboardComponent::rackArea() const
{
    auto area = getLocalBounds().reduced(12, 10);
    return area.removeFromBottom(70);
}

void PedalboardComponent::requestRebuild()
{
    if (rebuildPending)
        return;

    rebuildPending = true;
    juce::Component::SafePointer<PedalboardComponent> safe(this);
    juce::MessageManager::callAsync([safe]
    {
        if (safe == nullptr)
            return;
        safe->rebuildPending = false;
        safe->rebuild();
    });
}

void PedalboardComponent::pullUpPedal(const juce::String& id)
{
    if (auto fx = engine.getChain().findById(id))
        fx->setOnBoard(true);
    requestRebuild();
}

void PedalboardComponent::rackPedal(const juce::String& id)
{
    if (auto fx = engine.getChain().findById(id))
    {
        fx->setOnBoard(false);
        fx->setBypassed(true);
    }
    requestRebuild();
}

void PedalboardComponent::rebuild()
{
    pedals.clear();
    rack.clear();
    content.removeAllChildren();
    content.addAndMakeVisible(ampMarker);

    for (auto& fx : engine.getChain().getOrderedEffects())
    {
        if (fx == nullptr || fx->getId() == "preamp")
            continue;

        if (! fx->isOnBoard())
        {
            auto tile = std::make_unique<RackIconButton>(fx->getId(), fx->getDisplayName());
            const auto id = fx->getId();
            tile->onClick = [safe = juce::Component::SafePointer<PedalboardComponent>(this), id]
            {
                if (safe != nullptr)
                    safe->pullUpPedal(id);
            };
            addAndMakeVisible(*tile);
            rack.push_back(std::move(tile));
            continue;
        }

        auto pedal = std::make_unique<PedalComponent>(*fx, engine.getChain(), [this]
        {
            repaint();
        });
        content.addAndMakeVisible(*pedal);
        pedals.push_back(std::move(pedal));
    }

    resized();
    repaint();
}

void PedalboardComponent::paint(juce::Graphics& g)
{
    theme::studioFloor(g, getLocalBounds().toFloat());

    auto board = getLocalBounds().reduced(8, 6).toFloat();
    gfx::vinyl(g, board, juce::Colour(0xff1a1612), 12.0f);
    gfx::speckles(g, board, juce::Colour(0xffc9a45a).withAlpha(0.08f), 110, 2.0f, 5);
    g.setColour(juce::Colour(0xffc9a45a).withAlpha(0.18f));
    g.drawRoundedRectangle(board, 12.0f, 1.2f);

    auto rackBounds = rackArea().toFloat().expanded(6.0f, 4.0f);
    gfx::diamondPlate(g, rackBounds, juce::Colour(0xff3a3228));
    g.setColour(hoverRack ? juce::Colour(0xfff3e6c4).withAlpha(0.35f) : juce::Colours::white.withAlpha(0.08f));
    g.drawRoundedRectangle(rackBounds, 8.0f, hoverRack ? 2.0f : 1.0f);

    if (hoverIndex >= 0 && hoverIndex < (int) pedals.size() && ! hoverRack)
    {
        auto highlight = content.getLocalArea(&*pedals[(size_t) hoverIndex],
                                              pedals[(size_t) hoverIndex]->getLocalBounds());
        highlight = getLocalArea(&content, highlight);
        g.setColour(juce::Colour(0xfff3e6c4).withAlpha(0.18f));
        g.drawRoundedRectangle(highlight.toFloat().reduced(4.0f), 10.0f, 2.0f);
    }
}

void PedalboardComponent::resized()
{
    viewport.setBounds(boardArea());

    auto rackRow = rackArea();
    rackLabel.setBounds(rackRow.removeFromLeft(48));
    if (! rack.empty())
    {
        const int w = juce::jmax(64, rackRow.getWidth() / (int) rack.size());
        for (auto& tile : rack)
            tile->setBounds(rackRow.removeFromLeft(w).reduced(4, 4));
    }

    const int h = juce::jmax(1, viewport.getHeight());
    const int ampW = 148;
    int x = 4;
    bool ampPlaced = false;

    auto placeAmp = [&] ()
    {
        ampMarker.setBounds(x, 4, ampW, h - 8);
        x += ampW;
        ampPlaced = true;
    };

    for (auto& p : pedals)
    {
        const bool post = engine.getChain().slotLabelFor(p->getEffectId()) == "POST";
        if (! ampPlaced && post)
            placeAmp();

        const int w = p->preferredWidth(h);
        p->setBounds(x, 0, w, h);
        x += w;
    }

    if (! ampPlaced)
        placeAmp();

    content.setSize(juce::jmax(x + 4, viewport.getWidth()), h);
}

bool PedalboardComponent::isInterestedInDragSource(const SourceDetails&)
{
    return true;
}

bool PedalboardComponent::dropOverRack(const SourceDetails& details) const
{
    return rackArea().expanded(8, 12).contains(details.localPosition);
}

int PedalboardComponent::indexAtX(int x) const
{
    if (pedals.empty())
        return 0;

    const auto local = content.getLocalPoint(this, juce::Point<int>(x, boardArea().getCentreY()));
    int best = 0;
    int bestDist = std::numeric_limits<int>::max();
    for (int i = 0; i < (int) pedals.size(); ++i)
    {
        const int cx = pedals[(size_t) i]->getBounds().getCentreX();
        const int dist = std::abs(cx - local.x);
        if (dist < bestDist)
        {
            bestDist = dist;
            best = i;
        }
    }
    return best;
}

int PedalboardComponent::chainIndexForEffectId(const juce::String& id) const
{
    const auto ids = engine.getChain().getOrderIds();
    return ids.indexOf(id);
}

void PedalboardComponent::itemDragMove(const SourceDetails& details)
{
    hoverRack = dropOverRack(details);
    hoverIndex = hoverRack ? -1 : indexAtX(details.localPosition.x);
    repaint();
}

void PedalboardComponent::itemDragExit(const SourceDetails&)
{
    hoverIndex = -1;
    hoverRack = false;
    repaint();
}

void PedalboardComponent::itemDropped(const SourceDetails& details)
{
    auto* source = dynamic_cast<PedalComponent*>(details.sourceComponent.get());
    if (source == nullptr)
        return;

    if (dropOverRack(details))
    {
        rackPedal(source->getEffectId());
        hoverIndex = -1;
        hoverRack = false;
        return;
    }

    const int from = chainIndexForEffectId(source->getEffectId());
    const int visualTo = indexAtX(details.localPosition.x);
    int to = from;

    if (juce::isPositiveAndBelow(visualTo, (int) pedals.size()))
        to = chainIndexForEffectId(pedals[(size_t) visualTo]->getEffectId());

    if (from >= 0 && to >= 0)
        engine.getChain().moveEffect(from, to);

    hoverIndex = -1;
    hoverRack = false;
    rebuild();
}
