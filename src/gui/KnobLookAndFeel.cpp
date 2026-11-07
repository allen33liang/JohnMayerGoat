#include "KnobLookAndFeel.h"
#include "HardwareTheme.h"

KnobLookAndFeel::KnobLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, juce::Colours::whitesmoke);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
    setColour(juce::TextButton::textColourOffId, juce::Colours::whitesmoke);
}

void KnobLookAndFeel::applyKnobStyle(juce::Slider& slider, KnobStyle style)
{
    slider.getProperties().set("knobStyle", (int) style);
}

KnobStyle KnobLookAndFeel::styleFrom(const juce::Slider& slider)
{
    return (KnobStyle) (int) slider.getProperties().getWithDefault("knobStyle", (int) KnobStyle::Generic);
}

void KnobLookAndFeel::applyFootswitchStyle(juce::Button& button, FootswitchStyle style)
{
    button.getProperties().set("footswitchStyle", (int) style);
}

void KnobLookAndFeel::applyChrome(juce::Button& button, const juce::String& chromeId)
{
    button.getProperties().set("chrome", chromeId);
}

void KnobLookAndFeel::drawChickenHead(juce::Graphics& g, juce::Point<float> centre, float radius,
                                      float angle, juce::Colour body, juce::Colour pointer) const
{
    auto disc = juce::Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(centre);
    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.fillEllipse(disc.translated(0.0f, 1.6f));

    g.setColour(body.darker(0.35f));
    g.fillEllipse(disc);
    g.setColour(body);
    g.fillEllipse(disc.reduced(radius * 0.08f));
    for (int i = 0; i < 16; ++i)
    {
        const float a = (float) i / 16.0f * juce::MathConstants<float>::twoPi;
        g.setColour(juce::Colours::black.withAlpha(0.18f));
        g.drawLine(centre.x + std::cos(a) * radius * 0.72f,
                   centre.y + std::sin(a) * radius * 0.72f,
                   centre.x + std::cos(a) * radius * 0.92f,
                   centre.y + std::sin(a) * radius * 0.92f, 1.1f);
    }

    juce::Path ridge;
    const float inner = radius * 0.22f;
    const float outer = radius * 0.92f;
    ridge.addPieSegment(disc.reduced(radius * 0.18f), angle - 0.18f, angle + 0.18f, inner / outer);
    g.setColour(pointer);
    g.fillPath(ridge);

    juce::Path pointerPath;
    pointerPath.addRoundedRectangle(-radius * 0.13f, -radius * 0.92f, radius * 0.26f, radius * 0.72f, 2.0f);
    g.setColour(pointer);
    g.fillPath(pointerPath, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));

    g.setColour(body.brighter(0.15f));
    g.fillEllipse(centre.x - inner, centre.y - inner, inner * 2.0f, inner * 2.0f);
}

void KnobLookAndFeel::drawCappedKnob(juce::Graphics& g, juce::Point<float> centre, float radius,
                                     float angle, juce::Colour skirt, juce::Colour cap, juce::Colour pointer) const
{
    auto disc = juce::Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(centre);
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillEllipse(disc.translated(0.0f, 1.4f));

    juce::ColourGradient body(skirt.brighter(0.18f), disc.getTopLeft(), skirt.darker(0.22f), disc.getBottomRight(), false);
    g.setGradientFill(body);
    g.fillEllipse(disc);

    auto capDisc = disc.reduced(radius * 0.18f);
    juce::ColourGradient shine(cap.brighter(0.42f), capDisc.getTopLeft(), cap.darker(0.18f), capDisc.getBottomRight(), false);
    g.setGradientFill(shine);
    g.fillEllipse(capDisc);
    g.setColour(juce::Colours::white.withAlpha(0.35f));
    g.fillEllipse(capDisc.reduced(capDisc.getWidth() * 0.32f).translated(-capDisc.getWidth() * 0.1f, -capDisc.getHeight() * 0.14f));
    g.setColour(juce::Colours::black.withAlpha(0.22f));
    g.drawEllipse(capDisc, 1.0f);

    juce::Path pointerPath;
    pointerPath.addRoundedRectangle(-radius * 0.06f, -radius * 0.72f, radius * 0.12f, radius * 0.48f, 1.2f);
    g.setColour(pointer);
    g.fillPath(pointerPath, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
    g.setColour(cap.brighter(0.1f));
    g.fillEllipse(centre.x - radius * 0.08f, centre.y - radius * 0.08f, radius * 0.16f, radius * 0.16f);
}

void KnobLookAndFeel::drawTicks(juce::Graphics& g, juce::Point<float> centre, float radius,
                                float start, float end) const
{
    theme::ticks(g, centre, radius + 3.0f, radius + 8.0f, start, end, 11, theme::ivory().withAlpha(0.55f));
}

void KnobLookAndFeel::drawKnurled(juce::Graphics& g, juce::Point<float> centre, float radius, float angle) const
{
    auto disc = juce::Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(centre);
    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.fillEllipse(disc.translated(1.5f, 3.0f));
    juce::ColourGradient body(juce::Colour(0xff3a3a3a), disc.getTopLeft(),
                              juce::Colour(0xff111111), disc.getBottomRight(), false);
    g.setGradientFill(body);
    g.fillEllipse(disc);
    for (int i = 0; i < 24; ++i)
    {
        const float a = (float) i / 24.0f * juce::MathConstants<float>::twoPi;
        g.setColour(juce::Colours::white.withAlpha(0.12f));
        g.drawLine(centre.x + std::cos(a) * radius * 0.72f,
                   centre.y + std::sin(a) * radius * 0.72f,
                   centre.x + std::cos(a) * radius * 0.98f,
                   centre.y + std::sin(a) * radius * 0.98f, 1.6f);
    }
    juce::Path pointer;
    pointer.addRoundedRectangle(-1.6f, -radius * 0.82f, 3.2f, radius * 0.55f, 1.0f);
    g.setColour(theme::ivory());
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
}

void KnobLookAndFeel::drawSkirted(juce::Graphics& g, juce::Point<float> centre, float radius, float angle,
                                  float start, float end) const
{
    auto disc = juce::Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(centre);
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.fillEllipse(disc.translated(0.6f, 1.8f));

    juce::ColourGradient skirt(juce::Colour(0xff2a2a2a), disc.getTopLeft(),
                               juce::Colour(0xff0a0a0a), disc.getBottomRight(), false);
    g.setGradientFill(skirt);
    g.fillEllipse(disc);
    g.setColour(juce::Colour(0xff3a3a3a));
    g.drawEllipse(disc.reduced(0.6f), 1.0f);

    g.setColour(juce::Colours::white.withAlpha(0.92f));
    g.setFont(juce::Font(juce::FontOptions(juce::jmax(6.5f, radius * 0.20f), juce::Font::bold)));
    const float numR = radius * 0.82f;
    for (int i = 1; i <= 10; ++i)
    {
        const float t = (float) (i - 1) / 9.0f;
        const float a = start + t * (end - start) - juce::MathConstants<float>::halfPi;
        g.drawText(juce::String(i),
                   juce::Rectangle<float>(14.0f, 12.0f).withCentre({ centre.x + std::cos(a) * numR,
                                                                     centre.y + std::sin(a) * numR }),
                   juce::Justification::centred);
    }

    auto cap = disc.reduced(radius * 0.34f);
    juce::ColourGradient chrome(juce::Colour(0xfff4f4f4), cap.getTopLeft(),
                                juce::Colour(0xff8a8a8a), cap.getBottomRight(), false);
    g.setGradientFill(chrome);
    g.fillEllipse(cap);
    g.setColour(juce::Colours::white.withAlpha(0.45f));
    g.fillEllipse(cap.reduced(cap.getWidth() * 0.28f).translated(-cap.getWidth() * 0.08f, -cap.getHeight() * 0.12f));
    g.setColour(juce::Colours::black.withAlpha(0.28f));
    g.drawEllipse(cap, 1.0f);

    juce::Path pointer;
    pointer.addRoundedRectangle(-radius * 0.055f, -radius * 0.58f, radius * 0.11f, radius * 0.42f, 1.2f);
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
    g.setColour(juce::Colour(0xffd8d8d8));
    g.fillEllipse(centre.x - radius * 0.08f, centre.y - radius * 0.08f, radius * 0.16f, radius * 0.16f);
}

void KnobLookAndFeel::drawDotPointer(juce::Graphics& g, juce::Point<float> centre, float radius,
                                     float angle, juce::Colour body, juce::Colour pointer, bool triangle) const
{
    auto disc = juce::Rectangle<float>(radius * 2.0f, radius * 2.0f).withCentre(centre);
    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.fillEllipse(disc.translated(0.0f, 1.5f));
    g.setColour(body);
    g.fillEllipse(disc);
    g.setColour(body.brighter(0.12f));
    g.fillEllipse(disc.reduced(radius * 0.18f));

    if (triangle)
    {
        juce::Path tri;
        tri.addTriangle(-radius * 0.16f, -radius * 0.18f,
                        radius * 0.16f, -radius * 0.18f,
                        0.0f, -radius * 0.88f);
        g.setColour(pointer);
        g.fillPath(tri, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
    }
    else
    {
        juce::Path line;
        line.addRoundedRectangle(-1.5f, -radius * 0.82f, 3.0f, radius * 0.58f, 1.2f);
        g.setColour(pointer);
        g.fillPath(line, juce::AffineTransform::rotation(angle).translated(centre.x, centre.y));
    }
}

void KnobLookAndFeel::drawAmpToggle(juce::Graphics& g, juce::Rectangle<float> bounds, float sliderPos) const
{
    const bool on = sliderPos >= 0.5f;
    auto boot = bounds.reduced(4.0f);
    g.setColour(juce::Colour(0xff1a1210));
    g.fillRoundedRectangle(boot, 4.0f);
    g.setColour(juce::Colour(0xffc9a45a).withAlpha(0.4f));
    g.drawRoundedRectangle(boot, 4.0f, 1.0f);
    auto lever = juce::Rectangle<float>(boot.getWidth() * 0.45f, boot.getHeight() * 0.65f)
                     .withCentre({ boot.getCentreX(), on ? boot.getY() + boot.getHeight() * 0.3f : boot.getBottom() - boot.getHeight() * 0.3f });
    juce::ColourGradient chrome(juce::Colour(0xfff0ece4), lever.getTopLeft(),
                                juce::Colour(0xff8a8478), lever.getBottomRight(), false);
    g.setGradientFill(chrome);
    g.fillRoundedRectangle(lever, 3.0f);
}

void KnobLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                       juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height).reduced(3.0f);
    const auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto style = styleFrom(slider);
    if (style != KnobStyle::AmpSkirted && style != KnobStyle::AmpChrome && style != KnobStyle::AmpBlack && style != KnobStyle::AmpToggle)
        drawTicks(g, centre, radius, rotaryStartAngle, rotaryEndAngle);

    switch (style)
    {
        case KnobStyle::ChickenHead:
            drawChickenHead(g, centre, radius, angle, juce::Colour(0xff1c1c1c), juce::Colour(0xff3ad14a));
            break;
        case KnobStyle::SilverCap:
            drawCappedKnob(g, centre, radius, angle, juce::Colour(0xff1a1a1a), juce::Colour(0xffd8d8d8), juce::Colour(0xff222222));
            break;
        case KnobStyle::BluePointer:
            drawDotPointer(g, centre, radius, angle, juce::Colour(0xff1a1a1a), juce::Colour(0xff3ec7ff), false);
            break;
        case KnobStyle::WhitePointer:
            drawDotPointer(g, centre, radius, angle, juce::Colour(0xff161616), juce::Colours::white, true);
            break;
        case KnobStyle::Aqua:
            drawCappedKnob(g, centre, radius, angle, juce::Colour(0xff111111), juce::Colour(0xff1a1a1a), juce::Colour(0xffe8e8e8));
            break;
        case KnobStyle::AmpVintage:
            drawChickenHead(g, centre, radius, angle, juce::Colour(0xff1a1612), juce::Colour(0xffefe4c8));
            g.setColour(juce::Colour(0xffc9a45a).withAlpha(0.85f));
            g.drawEllipse(centre.x - radius * 0.22f, centre.y - radius * 0.22f, radius * 0.44f, radius * 0.44f, 1.4f);
            break;
        case KnobStyle::Bakelite:
            drawCappedKnob(g, centre, radius, angle, juce::Colour(0xff2a1c12), juce::Colour(0xff4a3424), juce::Colour(0xfff3e6c4));
            break;
        case KnobStyle::Knurled:
            drawKnurled(g, centre, radius, angle);
            break;
        case KnobStyle::AmpSkirted:
            drawSkirted(g, centre, radius, angle, rotaryStartAngle, rotaryEndAngle);
            break;
        case KnobStyle::AmpChrome:
            drawCappedKnob(g, centre, radius, angle, juce::Colour(0xffc8c8c8), juce::Colour(0xffececec), juce::Colour(0xff1a1a1a));
            break;
        case KnobStyle::AmpBlack:
            drawCappedKnob(g, centre, radius, angle, juce::Colour(0xff141414), juce::Colour(0xffd8d8d8), juce::Colour(0xff111111));
            break;
        case KnobStyle::AmpToggle:
            drawAmpToggle(g, bounds, sliderPos);
            break;
        case KnobStyle::Generic:
        default:
            drawCappedKnob(g, centre, radius, angle, juce::Colour(0xff1a1a1a), juce::Colour(0xff2a2a2a), juce::Colour(0xfff3e6c4));
            break;
    }
}

void KnobLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                           const juce::Colour& backgroundColour,
                                           bool shouldDrawButtonAsHighlighted,
                                           bool shouldDrawButtonAsDown)
{
    if (backgroundColour.getAlpha() == 0)
        return;

    const auto foot = (FootswitchStyle) (int) button.getProperties().getWithDefault("footswitchStyle", 0);
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);

    if (foot == FootswitchStyle::Metal || foot == FootswitchStyle::Rubber)
    {
        const auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
        auto sw = juce::Rectangle<float>(size, size).withCentre(bounds.getCentre()).reduced(2.0f);
        const bool down = shouldDrawButtonAsDown || button.getToggleState();

        g.setColour(juce::Colours::black.withAlpha(0.35f));
        g.fillEllipse(sw.translated(0.0f, 2.0f));

        if (foot == FootswitchStyle::Rubber)
        {
            g.setColour(down ? juce::Colour(0xff2a2a2a) : juce::Colour(0xff111111));
            g.fillRoundedRectangle(sw, 6.0f);
            g.setColour(juce::Colours::white.withAlpha(0.08f));
            for (int r = 0; r < 3; ++r)
                g.drawRoundedRectangle(sw.reduced((float) r * 4.0f), 5.0f, 1.0f);
        }
        else
        {
            juce::ColourGradient metal(juce::Colour(0xffefefef), sw.getTopLeft(),
                                       juce::Colour(0xff8a8a8a), sw.getBottomRight(), false);
            g.setGradientFill(metal);
            g.fillEllipse(sw);
            g.setColour(juce::Colours::black.withAlpha(0.25f));
            g.drawEllipse(sw, 1.2f);
            g.setColour(juce::Colour(0xffcfcfcf));
            g.fillEllipse(sw.reduced(sw.getWidth() * 0.28f));
            if (down)
            {
                g.setColour(juce::Colours::black.withAlpha(0.18f));
                g.fillEllipse(sw);
            }
        }
        return;
    }

    const auto chrome = button.getProperties().getWithDefault("chrome", "steel").toString();
    juce::Colour fill, edge, top;

    if (chrome == "gold")
    {
        fill = juce::Colour(0xffc9a45a);
        edge = juce::Colour(0xfff3e6c4);
        top = juce::Colour(0xffe8d7a4);
    }
    else if (chrome == "red")
    {
        fill = juce::Colour(0xff6a1e1a);
        edge = juce::Colour(0xffd46a5c);
        top = juce::Colour(0xff8a2e28);
    }
    else if (chrome == "teal")
    {
        fill = juce::Colour(0xff1d5a62);
        edge = juce::Colour(0xff7ad0d8);
        top = juce::Colour(0xff2d7a84);
    }
    else if (chrome == "green")
    {
        fill = juce::Colour(0xff2a5a32);
        edge = juce::Colour(0xff8fd49a);
        top = juce::Colour(0xff3a7a42);
    }
    else if (chrome == "cream")
    {
        fill = juce::Colour(0xffd8c49a);
        edge = juce::Colour(0xfff3e6c4);
        top = juce::Colour(0xffefe0c0);
    }
    else if (chrome == "blue")
    {
        fill = juce::Colour(0xff24364a);
        edge = juce::Colour(0xff7aa4d4);
        top = juce::Colour(0xff34506a);
    }
    else
    {
        fill = juce::Colour(0xff2c2a28);
        edge = juce::Colour(0xff8a8070);
        top = juce::Colour(0xff3a3834);
    }

    if (button.getToggleState())
        fill = fill.brighter(0.18f);
    if (shouldDrawButtonAsDown)
        fill = fill.darker(0.12f);
    else if (shouldDrawButtonAsHighlighted)
        fill = fill.brighter(0.1f);

    juce::ColourGradient grad(top, bounds.getTopLeft(), fill, bounds.getBottomLeft(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(bounds, 8.0f);
    g.setColour(edge.withAlpha(button.getToggleState() ? 0.9f : 0.45f));
    g.drawRoundedRectangle(bounds, 8.0f, 1.4f);
}

void KnobLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                     bool, bool)
{
    const auto foot = (FootswitchStyle) (int) button.getProperties().getWithDefault("footswitchStyle", 0);
    if (foot != FootswitchStyle::None)
        return;

    const auto chrome = button.getProperties().getWithDefault("chrome", "steel").toString();
    const bool darkText = chrome == "gold" || chrome == "cream";
    g.setColour(darkText ? juce::Colour(0xff1a120c) : juce::Colour(0xfff7f1e4));
    g.setFont(juce::Font(juce::FontOptions("Avenir Next Condensed", 13.0f, juce::Font::bold)));
    g.drawFittedText(button.getButtonText(), button.getLocalBounds().reduced(2), juce::Justification::centred, 1);
}
