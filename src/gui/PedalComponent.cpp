#include "PedalComponent.h"
#include "GraphicUtils.h"
#include "HardwareTheme.h"
#include "PedalboardComponent.h"

namespace
{
juce::Rectangle<int> centredSquare(juce::Rectangle<int> r)
{
    const int s = juce::jmin(r.getWidth(), r.getHeight());
    return r.withSizeKeepingCentre(s, s);
}
}

PedalComponent::PedalComponent(Effect& effectToControl, EffectChain& chainToQuery, std::function<void()> changed)
    : effect(effectToControl), chain(chainToQuery), onChanged(std::move(changed))
{
    setName(effect.getId());
    setOpaque(false);

    bypassButton.setClickingTogglesState(true);
    bypassButton.setButtonText({});
    bypassButton.setLookAndFeel(&knobsLaf);
    KnobLookAndFeel::applyFootswitchStyle(bypassButton, footswitchStyleForEffect());
    bypassButton.setToggleState(! effect.isBypassed(), juce::dontSendNotification);
    bypassButton.onClick = [this]
    {
        effect.setBypassed(! bypassButton.getToggleState());
        led.setOn(! effect.isBypassed());
        repaint();
        if (onChanged)
            onChanged();
    };
    addAndMakeVisible(bypassButton);
    led.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(led);

    typeToggle.setVisible(effect.getId() == "reverb");
    typeToggle.onClick = [this]
    {
        const int next = (typeToggle.getIndex() + 1) % 3;
        typeToggle.setIndex(next);
        effect.setParameter("type", next <= 0 ? 0.0f : (next == 1 ? 0.5f : 1.0f));
        if (onChanged)
            onChanged();
        repaint();
    };
    addAndMakeVisible(typeToggle);

    const auto style = knobStyleForEffect();
    const auto textColour = labelColour();
    for (const auto& p : effect.getParamList())
    {
        if (p.id == "type")
            continue;

        KnobCtrl ctrl;
        ctrl.id = p.id;
        ctrl.slider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
        ctrl.slider->setRange(0.0, 1.0, 0.001);
        ctrl.slider->setValue(effect.getParameter(p.id), juce::dontSendNotification);
        ctrl.slider->setLookAndFeel(&knobsLaf);
        ctrl.slider->setRotaryParameters(juce::degreesToRadians(225.0f), juce::degreesToRadians(495.0f), true);
        ctrl.slider->setMouseDragSensitivity(180);
        ctrl.slider->setVelocityBasedMode(true);
        ctrl.slider->setVelocityModeParameters(0.4, 1, 0.09, true);
        ctrl.slider->setDoubleClickReturnValue(true, p.defaultValue);
        KnobLookAndFeel::applyKnobStyle(*ctrl.slider, style);
        const auto paramId = p.id;
        ctrl.slider->onValueChange = [this, paramId, s = ctrl.slider.get()]
        {
            effect.setParameter(paramId, (float) s->getValue());
            liveSlider = s;
            valueReadout.setText(juce::String(juce::roundToInt(s->getValue() * 100.0)), juce::dontSendNotification);
            valueReadout.setBounds(s->getBounds().withSizeKeepingCentre(36, 18));
            valueReadout.setVisible(true);
            valueReadout.toFront(false);
        };
        ctrl.slider->onDragEnd = [this]
        {
            liveSlider = nullptr;
            valueReadout.setVisible(false);
        };
        addAndMakeVisible(*ctrl.slider);

        ctrl.label = std::make_unique<juce::Label>();
        ctrl.label->setText(p.label.toUpperCase(), juce::dontSendNotification);
        ctrl.label->setJustificationType(juce::Justification::centred);
        ctrl.label->setColour(juce::Label::textColourId, textColour);
        const float labelSize = (effect.getId() == "bluesbreaker" || effect.getId() == "shredmaster"
                                 || effect.getId() == "rat") ? 18.0f : 15.0f;
        ctrl.label->setFont(gfx::fontCondensed(labelSize));
        ctrl.label->setInterceptsMouseClicks(false, false);
        addAndMakeVisible(*ctrl.label);
        knobs.push_back(std::move(ctrl));
    }

    valueReadout.setJustificationType(juce::Justification::centred);
    valueReadout.setColour(juce::Label::textColourId, juce::Colour(0xfff3e6c4));
    valueReadout.setColour(juce::Label::backgroundColourId, juce::Colour(0xcc101114));
    valueReadout.setFont(gfx::fontCondensed(11.0f));
    valueReadout.setInterceptsMouseClicks(false, false);
    valueReadout.setVisible(false);
    addAndMakeVisible(valueReadout);

    refreshFromEffect();
}

PedalComponent::~PedalComponent()
{
    bypassButton.setLookAndFeel(nullptr);
    for (auto& k : knobs)
        k.slider->setLookAndFeel(nullptr);
}

void PedalComponent::refreshFromEffect()
{
    bypassButton.setToggleState(! effect.isBypassed(), juce::dontSendNotification);
    led.setOn(! effect.isBypassed());
    for (auto& k : knobs)
        k.slider->setValue(effect.getParameter(k.id), juce::dontSendNotification);
    if (effect.getId() == "reverb")
    {
        const float t = effect.getParameter("type");
        typeToggle.setIndex(t < 0.33f ? 0 : (t < 0.66f ? 1 : 2));
        typeToggle.setVisible(true);
    }
    else
    {
        typeToggle.setVisible(false);
    }
    repaint();
}

int PedalComponent::preferredWidth(int availableHeight) const
{
    const auto id = effect.getId();
    const float ratio = id == "reverb" ? 0.52f
                       : id == "shredmaster" ? 0.48f
                       : id == "slapback" ? 0.46f
                       : 0.42f;
    return juce::jlimit(150, 230, juce::roundToInt((float) availableHeight * ratio));
}

juce::Rectangle<int> PedalComponent::chassisBounds() const
{
    return getLocalBounds().reduced(6);
}

juce::Slider* PedalComponent::sliderFor(const juce::String& paramId) const
{
    for (auto& k : knobs)
        if (k.id == paramId)
            return k.slider.get();
    return nullptr;
}

juce::Label* PedalComponent::labelFor(const juce::String& paramId) const
{
    for (auto& k : knobs)
        if (k.id == paramId)
            return k.label.get();
    return nullptr;
}

KnobStyle PedalComponent::knobStyleForEffect() const
{
    const auto id = effect.getId();
    if (id == "tubeScreamer") return KnobStyle::ChickenHead;
    if (id == "bluesbreaker") return KnobStyle::BluePointer;
    if (id == "shredmaster") return KnobStyle::WhitePointer;
    if (id == "rat") return KnobStyle::Bakelite;
    if (id == "chorus") return KnobStyle::SilverCap;
    if (id == "delay") return KnobStyle::Knurled;
    if (id == "slapback") return KnobStyle::Aqua;
    if (id == "reverb") return KnobStyle::SilverCap;
    return KnobStyle::Generic;
}

FootswitchStyle PedalComponent::footswitchStyleForEffect() const
{
    const auto id = effect.getId();
    if (id == "tubeScreamer" || id == "chorus")
        return FootswitchStyle::Rubber;
    return FootswitchStyle::Metal;
}

juce::Colour PedalComponent::labelColour() const
{
    const auto id = effect.getId();
    if (id == "tubeScreamer") return juce::Colour(0xff163016);
    if (id == "bluesbreaker") return juce::Colours::white;
    if (id == "chorus") return juce::Colour(0xff163042);
    if (id == "reverb") return juce::Colour(0xff2a2418);
    if (id == "slapback") return juce::Colour(0xffe8f7ff);
    return juce::Colours::white.withAlpha(0.9f);
}

void PedalComponent::placeLed(juce::Rectangle<int> bounds)
{
    led.setBounds(bounds);
    led.toFront(false);
}

void PedalComponent::placeKnob(const juce::String& paramId, juce::Rectangle<int> knobArea, juce::Rectangle<int> labelArea)
{
    if (auto* s = sliderFor(paramId))
        s->setBounds(centredSquare(knobArea));
    if (auto* l = labelFor(paramId))
    {
        if (labelArea.isEmpty())
        {
            l->setVisible(false);
            l->setBounds({});
        }
        else
        {
            l->setVisible(true);
            l->setBounds(labelArea);
        }
    }
}

void PedalComponent::layoutKnobs(juce::Rectangle<int> chassis)
{
    const auto id = effect.getId();

    auto rowOf = [this] (juce::Rectangle<int> row, const std::vector<juce::String>& ids, bool labelsAbove)
    {
        if (ids.empty())
            return;
        const int w = row.getWidth() / (int) ids.size();
        for (const auto& pid : ids)
        {
            auto col = row.removeFromLeft(w).reduced(2, 0);
            auto label = labelsAbove ? col.removeFromTop(20) : col.removeFromBottom(20);
            placeKnob(pid, col.reduced(1), label);
        }
    };

    if (id == "tubeScreamer")
    {
        auto top = chassis.removeFromTop(juce::roundToInt((float) chassis.getHeight() * 0.40f)).reduced(8, 4);
        const int w = top.getWidth() / 3;
        for (const auto& pid : { juce::String("drive"), juce::String("tone"), juce::String("level") })
        {
            auto col = top.removeFromLeft(w).reduced(1, 0);
            auto knob = col.removeFromTop(col.getHeight() - 20);
            placeKnob(pid, knob.reduced(1), col);
        }
        auto pad = chassis.reduced(18, 10).removeFromBottom(juce::roundToInt((float) chassis.getHeight() * 0.42f));
        bypassButton.setBounds(pad.withSizeKeepingCentre(juce::jmin(pad.getWidth(), 86), juce::jmin(pad.getHeight(), 70)));
        placeLed({ chassisBounds().getCentreX() - 5, chassisBounds().getY() + 10, 10, 10 });
        return;
    }

    if (id == "bluesbreaker")
    {
        auto ridge = chassis.removeFromTop(juce::roundToInt((float) chassis.getHeight() * 0.30f)).reduced(8, 4);
        rowOf(ridge, { "drive", "tone", "level" }, false);
        auto body = chassis.reduced(8, 10);
        placeLed({ body.getCentreX() - 6, body.getY() + 8, 12, 12 });
        bypassButton.setBounds(body.removeFromBottom(72).withSizeKeepingCentre(44, 44));
        return;
    }

    if (id == "rat")
    {
        auto top = chassis.removeFromTop(juce::roundToInt((float) chassis.getHeight() * 0.36f)).reduced(8, 4);
        rowOf(top, { "distortion", "filter", "volume" }, false);
        auto body = chassis.reduced(8, 8);
        placeLed({ body.getCentreX() - 6, body.getY() + 10, 12, 12 });
        bypassButton.setBounds(body.removeFromBottom(64).withSizeKeepingCentre(42, 42));
        return;
    }

    if (id == "shredmaster")
    {
        auto top = chassis.removeFromTop(juce::roundToInt((float) chassis.getHeight() * 0.32f)).reduced(4, 4);
        rowOf(top, { "gain", "bass", "treble", "volume" }, false);
        auto body = chassis.reduced(8, 10);
        placeLed({ body.getCentreX() - 6, body.getY() + 8, 12, 12 });
        bypassButton.setBounds(body.removeFromBottom(72).withSizeKeepingCentre(44, 44));
        return;
    }

    if (id == "chorus")
    {
        auto top = chassis.removeFromTop(juce::roundToInt((float) chassis.getHeight() * 0.36f)).reduced(12, 8);
        rowOf(top, { "rate", "depth" }, false);
        if (auto* mix = sliderFor("mix"))
        {
            mix->setVisible(false);
            if (auto* l = labelFor("mix"))
                l->setVisible(false);
        }
        auto pad = chassis.removeFromBottom(juce::roundToInt((float) chassis.getHeight() * 0.42f)).reduced(16, 8);
        bypassButton.setBounds(pad.withSizeKeepingCentre(juce::jmin(pad.getWidth(), 90), juce::jmin(pad.getHeight(), 54)));
        placeLed({ chassisBounds().getCentreX() - 5, chassisBounds().getY() + 10, 10, 10 });
        return;
    }

    if (id == "delay")
    {
        auto top = chassis.removeFromTop(juce::roundToInt((float) chassis.getHeight() * 0.42f)).reduced(10, 8);
        auto vol = top.removeFromTop(top.getHeight() / 2);
        auto volLabel = vol.removeFromBottom(18);
        placeKnob("mix", vol, volLabel);

        auto lower = top;
        auto left = lower.removeFromLeft(lower.getWidth() / 2);
        auto leftLabel = left.removeFromBottom(18);
        auto rightLabel = lower.removeFromBottom(18);
        placeKnob("feedback", left.reduced(4), leftLabel);
        placeKnob("time", lower.reduced(4), rightLabel);
        bypassButton.setBounds(chassis.removeFromBottom(56).withSizeKeepingCentre(42, 42));
        placeLed({ chassisBounds().getCentreX() - 6, chassisBounds().getCentreY() - 6, 12, 12 });
        return;
    }

    if (id == "slapback")
    {
        auto top = chassis.removeFromTop(juce::roundToInt((float) chassis.getHeight() * 0.40f)).reduced(8, 6);
        auto left = top.removeFromLeft(top.getWidth() / 3);
        auto right = top.removeFromRight(top.getWidth() / 2);
        auto leftLabel = left.removeFromBottom(18);
        auto rightLabel = right.removeFromBottom(18);
        placeKnob("delay", left.reduced(2), leftLabel);
        placeKnob("blend", right.reduced(2), rightLabel);

        auto mid = top;
        auto midLabel = juce::Rectangle<int>(mid.getX(), mid.getBottom() - 2, mid.getWidth(), 18);
        placeKnob("feedback", mid.withSizeKeepingCentre(48, 48).translated(0, 8), midLabel);
        bypassButton.setBounds(chassis.removeFromBottom(56).withSizeKeepingCentre(40, 40));
        placeLed({ chassisBounds().getCentreX() - 6, chassisBounds().getY() + 12, 12, 12 });
        return;
    }

    if (id == "reverb")
    {
        auto gold = chassis.removeFromTop(juce::roundToInt((float) chassis.getHeight() * 0.68f)).reduced(6, 4);
        auto top = gold.removeFromTop(juce::roundToInt((float) gold.getHeight() * 0.38f));
        rowOf(top, { "decay", "predelay" }, true);
        gold.removeFromBottom(36);
        auto mixCol = juce::Rectangle<int>(0, 0, 78, 92).withCentre(gold.getCentre().translated(0, -4));
        auto mixLabel = mixCol.removeFromTop(16);
        placeKnob("mix", mixCol, mixLabel);

        auto silver = chassis.reduced(6, 4);
        typeToggle.setBounds(silver.removeFromLeft(78).reduced(2, 2));
        bypassButton.setBounds(silver.withSizeKeepingCentre(42, 42));
        placeLed({ chassisBounds().getCentreX() - 5, chassisBounds().getY() + 12, 10, 10 });
        return;
    }

    if (! knobs.empty())
    {
        auto row = chassis.removeFromTop(juce::roundToInt((float) chassis.getHeight() * 0.4f));
        std::vector<juce::String> ids;
        for (auto& k : knobs)
            ids.push_back(k.id);
        rowOf(row, ids, false);
        bypassButton.setBounds(chassis.removeFromBottom(56).withSizeKeepingCentre(40, 40));
    }
}

void PedalComponent::resized()
{
    layoutKnobs(chassisBounds());
    led.toFront(false);
    typeToggle.toFront(false);
    if (liveSlider != nullptr)
        valueReadout.setBounds(liveSlider->getBounds().withSizeKeepingCentre(36, 18));
}

void PedalComponent::paintLed(juce::Graphics& g, juce::Rectangle<float> ledBounds, bool on) const
{
    gfx::paintLed(g, ledBounds, on);
}

void PedalComponent::LedDot::paint(juce::Graphics& g)
{
    gfx::paintLed(g, getLocalBounds().toFloat(), lit);
}

void PedalComponent::paintTubeScreamer(juce::Graphics& g, juce::Rectangle<float> b)
{
    theme::dropShadow(g, b, 10.0f, 7.0f);
    gfx::vinyl(g, b, juce::Colour(0xff3aaa3a), 10.0f);
    gfx::pinstripes(g, b, juce::Colour(0xff2d8a2d).withAlpha(0.45f), 5.0f, 0.0f);
    gfx::speckles(g, b, juce::Colour(0xff1f6b1f).withAlpha(0.18f), 70, 1.4f, 4);
    theme::topLeftSheen(g, b, 10.0f);
    g.setColour(juce::Colour(0xff2d8a2d));
    g.drawRoundedRectangle(b.reduced(1.0f), 10.0f, 2.0f);

    auto head = b.removeFromTop(b.getHeight() * 0.42f).reduced(6.0f, 6.0f);
    gfx::vinyl(g, head, juce::Colour(0xff46b846), 6.0f);
    gfx::speckles(g, head, juce::Colours::white.withAlpha(0.08f), 40, 1.1f, 8);
    theme::topLeftSheen(g, head, 6.0f);

    auto plate = b.removeFromTop(36.0f).reduced(8.0f, 2.0f);
    g.setColour(juce::Colour(0xff1c1c1c));
    g.fillRect(plate);
    gfx::brushed(g, plate, juce::Colour(0xff3a3a3a), true, 5);
    g.setColour(theme::ivory());
    g.setFont(gfx::fontHelveticaBold(13.0f));
    g.drawText("TUBE SCREAMER", plate, juce::Justification::centred);

    auto pad = b.reduced(12.0f, 8.0f);
    g.setColour(juce::Colour(0xff111111));
    g.fillRoundedRectangle(pad, 6.0f);
    gfx::chevrons(g, pad, juce::Colour(0xff2a2a2a).withAlpha(0.7f), 8.0f);
}

void PedalComponent::paintBluesBreaker(juce::Graphics& g, juce::Rectangle<float> b)
{
    gfx::vinyl(g, b, juce::Colour(0xff161412), 8.0f);
    gfx::pinstripes(g, b, juce::Colour(0xff2aa4ff).withAlpha(0.28f), 7.0f, 16.0f);
    gfx::scratches(g, b, juce::Colours::white, 8, 21);

    auto ridge = b.removeFromTop(b.getHeight() * 0.30f);
    gfx::vinyl(g, ridge, juce::Colour(0xff1e1a18), 6.0f);
    g.setColour(juce::Colour(0xff0e0c0b));
    g.fillRect(juce::Rectangle<float>(ridge.getX(), ridge.getBottom() - 8.0f, ridge.getWidth(), 10.0f));

    auto name = juce::Rectangle<float>(b.getX() + 8.0f, b.getY() + 28.0f, b.getWidth() - 16.0f, 36.0f);
    g.setColour(juce::Colour(0xff2aa4ff));
    g.setFont(gfx::fontFutura(18.0f));
    g.drawFittedText("BLUES BREAKER", name.toNearestInt(), juce::Justification::centred, 2);
}

void PedalComponent::paintRat(juce::Graphics& g, juce::Rectangle<float> b)
{
    gfx::vinyl(g, b, juce::Colour(0xff111111), 6.0f);
    gfx::diamondPlate(g, b.reduced(8.0f, 36.0f), juce::Colour(0xff2a2a2a));
    gfx::scratches(g, b, juce::Colours::white, 12, 14);
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawRoundedRectangle(b.reduced(1.5f), 6.0f, 1.5f);
    gfx::screws(g, b, 7.0f, 6.0f);

    auto badge = juce::Rectangle<float>(0.0f, 0.0f, 92.0f, 30.0f)
                     .withCentre({ b.getCentreX(), b.getY() + b.getHeight() * 0.58f });
    g.setColour(juce::Colours::white);
    g.drawRect(badge, 2.0f);
    g.setFont(gfx::fontImpact(20.0f));
    g.drawText("RAT", badge, juce::Justification::centred);
}

void PedalComponent::paintShredmaster(juce::Graphics& g, juce::Rectangle<float> b)
{
    gfx::vinyl(g, b, juce::Colour(0xff101010), 8.0f);
    gfx::pinstripes(g, b, juce::Colour(0xffd8c89a).withAlpha(0.28f), 6.0f, 0.0f);
    gfx::scratches(g, b, juce::Colours::white, 9, 18);

    auto ridge = b.removeFromTop(b.getHeight() * 0.32f);
    gfx::vinyl(g, ridge, juce::Colour(0xff171717), 6.0f);
    gfx::brushed(g, ridge, juce::Colour(0xff2a2a2a), true, 4);

    auto name = juce::Rectangle<float>(b.getX() + 8.0f, b.getY() + 28.0f, b.getWidth() - 16.0f, 36.0f);
    g.setColour(juce::Colour(0xffd8c89a));
    g.setFont(gfx::fontCopperplate(18.0f));
    g.drawFittedText("SHRED MASTER", name.toNearestInt(), juce::Justification::centred, 2);
}

void PedalComponent::paintChorus(juce::Graphics& g, juce::Rectangle<float> b)
{
    gfx::vinyl(g, b, juce::Colour(0xff4ec2e8), 8.0f);
    gfx::pinstripes(g, b, juce::Colours::white.withAlpha(0.28f), 6.0f, 28.0f);
    gfx::scratches(g, b, juce::Colour(0xff1a5a70), 8, 6);
    g.setColour(juce::Colour(0xff3aa9cc));
    g.drawRoundedRectangle(b.reduced(1.0f), 8.0f, 1.5f);

    auto name = juce::Rectangle<float>(b.getX(), b.getCentreY() - 10.0f, b.getWidth(), 28.0f);
    g.setColour(juce::Colour(0xff111111));
    g.setFont(gfx::fontGeorgiaBold(18.0f));
    g.drawText("CHORUS", name, juce::Justification::centred);

    auto pad = b.removeFromBottom(b.getHeight() * 0.28f).reduced(10.0f, 8.0f);
    g.setColour(juce::Colour(0xff111111));
    g.fillRoundedRectangle(pad, 4.0f);
    gfx::weave(g, pad, juce::Colour(0xff2a2a2a).withAlpha(0.5f), juce::Colour(0xff050505).withAlpha(0.4f), 7.0f);
}

void PedalComponent::paintEchoplex(juce::Graphics& g, juce::Rectangle<float> b)
{
    gfx::anodized(g, b, juce::Colour(0xff161616), 6.0f);

    g.saveState();
    juce::Path clip;
    clip.addRoundedRectangle(b.reduced(2.0f), 6.0f);
    g.reduceClipRegion(clip);
    const float maxR = juce::jmax(b.getWidth(), b.getHeight()) * 0.92f;
    gfx::rings(g, b.getCentre(), maxR, juce::Colours::white.withAlpha(0.32f), 10);
    g.restoreState();

    gfx::scratches(g, b, juce::Colours::white, 10, 33);
    g.setColour(juce::Colour(0xffcfcfcf));
    g.drawRoundedRectangle(b.reduced(4.0f), 4.0f, 1.2f);
    gfx::screws(g, b, 10.0f, 7.0f);

    auto logo = juce::Rectangle<float>(0, 0, b.getWidth() - 16.0f, 36.0f)
                    .withCentre({ b.getCentreX(), b.getY() + b.getHeight() * 0.66f });
    g.setColour(juce::Colours::white);
    g.setFont(gfx::fontCopperplate(15.0f));
    g.drawText("ECHOPLEX", logo, juce::Justification::centred);
}

void PedalComponent::paintAquaPuss(juce::Graphics& g, juce::Rectangle<float> b)
{
    gfx::anodized(g, b, juce::Colour(0xff1aa7d0), 8.0f);
    gfx::waves(g, b, juce::Colours::white.withAlpha(0.28f), 4.5f, 15.0f);
    g.setColour(juce::Colour(0xff0e6f8c).withAlpha(0.5f));
    g.drawRoundedRectangle(b.reduced(1.0f), 8.0f, 1.4f);
    gfx::screws(g, b, 8.0f, 6.0f);

    g.setColour(juce::Colour(0xffeaf7ff));
    g.setFont(gfx::fontFutura(18.0f));
    auto name = juce::Rectangle<float>(b.getX(), b.getY() + b.getHeight() * 0.46f, b.getWidth(), 32.0f);
    g.drawText("AQUA-PUSS", name, juce::Justification::centred);
}

void PedalComponent::paintGolden(juce::Graphics& g, juce::Rectangle<float> b)
{
    gfx::anodized(g, b, juce::Colour(0xffcfc6b8), 10.0f);
    gfx::screws(g, b, 8.0f, 6.5f);

    auto gold = b.removeFromTop(b.getHeight() * 0.68f).reduced(5.0f, 5.0f);
    gfx::vinyl(g, gold, juce::Colour(0xffc4b08a), 6.0f);
    gfx::pinstripes(g, gold, juce::Colour(0xff8a7048).withAlpha(0.32f), 7.0f, 12.0f);

    g.setColour(juce::Colour(0xff2a2418));
    g.setFont(gfx::fontDidot(32.0f));
    auto name = gold.removeFromBottom(54.0f);
    g.drawText("GOLDEN", name.removeFromTop(32.0f), juce::Justification::centred);
    g.setFont(gfx::fontCondensed(15.0f));
    g.drawText("REVERBERATOR", name, juce::Justification::centred);

    auto silver = b.reduced(4.0f, 2.0f);
    gfx::anodized(g, silver, juce::Colour(0xffd8d8d8), 6.0f);
}

void PedalComponent::paint(juce::Graphics& g)
{
    auto chassis = chassisBounds().toFloat();
    const auto id = effect.getId();

    g.setColour(juce::Colours::black.withAlpha(0.28f));
    g.fillRoundedRectangle(chassis.translated(1.5f, 4.0f), 10.0f);

    if (id == "tubeScreamer") paintTubeScreamer(g, chassis);
    else if (id == "bluesbreaker") paintBluesBreaker(g, chassis);
    else if (id == "rat") paintRat(g, chassis);
    else if (id == "shredmaster") paintShredmaster(g, chassis);
    else if (id == "chorus") paintChorus(g, chassis);
    else if (id == "delay") paintEchoplex(g, chassis);
    else if (id == "slapback") paintAquaPuss(g, chassis);
    else if (id == "reverb") paintGolden(g, chassis);
    else
    {
        g.setColour(effect.getColour());
        g.fillRoundedRectangle(chassis, 10.0f);
    }
}

void PedalComponent::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isLeftButtonDown() && e.originalComponent == this)
        if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
            container->startDragging(effect.getId(), this, juce::ScaledImage(), true, nullptr, &e.source);
}

void PedalComponent::mouseDrag(const juce::MouseEvent&) {}

bool PedalComponent::isInterestedInDragSource(const SourceDetails&)
{
    return true;
}

juce::DragAndDropTarget::SourceDetails mappedDetails(juce::Component* target, const juce::DragAndDropTarget::SourceDetails& details)
{
    auto* board = target->findParentComponentOfClass<PedalboardComponent>();
    if (board == nullptr)
        return details;
    return { details.description, details.sourceComponent, board->getLocalPoint(target, details.localPosition) };
}

void PedalComponent::itemDropped(const SourceDetails& details)
{
    if (auto* board = findParentComponentOfClass<PedalboardComponent>())
        board->itemDropped(mappedDetails(this, details));
}

void PedalComponent::itemDragMove(const SourceDetails& details)
{
    if (auto* board = findParentComponentOfClass<PedalboardComponent>())
        board->itemDragMove(mappedDetails(this, details));
}
