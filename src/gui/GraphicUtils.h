#pragma once

#include <JuceHeader.h>
#include "HardwareTheme.h"
#include <cmath>

namespace gfx
{
inline void speckles(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour colour, int count, float size, int seed)
{
    juce::Random rng((juce::int64) seed * 7919 + (juce::int64) (r.getX() * 13 + r.getY() * 29));
    for (int i = 0; i < count; ++i)
    {
        const float x = r.getX() + rng.nextFloat() * r.getWidth();
        const float y = r.getY() + rng.nextFloat() * r.getHeight();
        const float s = size * (0.4f + rng.nextFloat());
        g.setColour(colour.withAlpha(colour.getFloatAlpha() * (0.25f + rng.nextFloat() * 0.75f)));
        g.fillEllipse(x, y, s, s * (0.7f + rng.nextFloat() * 0.6f));
    }
}

inline void brushed(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour colour, bool horizontal, int seed = 1)
{
    juce::Random rng(seed + (int) r.getWidth() * 17);
    const int lines = juce::jmax(12, (int) ((horizontal ? r.getHeight() : r.getWidth()) * 0.9f));
    for (int i = 0; i < lines; ++i)
    {
        const float t = (float) i / (float) lines;
        const float a = 0.03f + rng.nextFloat() * 0.07f;
        g.setColour((rng.nextBool() ? colour.brighter(0.25f) : colour.darker(0.25f)).withAlpha(a));
        if (horizontal)
        {
            const float y = r.getY() + t * r.getHeight() + rng.nextFloat();
            g.drawLine(r.getX(), y, r.getRight(), y, 1.0f);
        }
        else
        {
            const float x = r.getX() + t * r.getWidth() + rng.nextFloat();
            g.drawLine(x, r.getY(), x, r.getBottom(), 1.0f);
        }
    }
}

inline void tolex(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour colour, float radius)
{
    juce::ColourGradient grad(colour.brighter(0.12f), r.getTopLeft(), colour.darker(0.28f), r.getBottomRight(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(r, radius);
    speckles(g, r, colour.brighter(0.2f).withAlpha(0.18f), 220, 2.2f, 11);
    speckles(g, r, colour.darker(0.35f).withAlpha(0.2f), 180, 1.8f, 23);
}

inline void anodized(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour colour, float radius)
{
    juce::ColourGradient grad(colour.brighter(0.18f), r.getTopLeft(), colour.darker(0.22f), r.getBottomRight(), false);
    g.setGradientFill(grad);
    g.fillRoundedRectangle(r, radius);
    brushed(g, r, colour, true, 7);
    speckles(g, r, juce::Colours::white.withAlpha(0.07f), 80, 1.2f, 3);
}

inline void vinyl(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour colour, float radius)
{
    g.setColour(colour);
    g.fillRoundedRectangle(r, radius);
    juce::ColourGradient sheen(colour.brighter(0.16f).withAlpha(0.35f), r.getTopLeft(),
                               juce::Colours::transparentBlack, r.getBottomRight(), false);
    g.setGradientFill(sheen);
    g.fillRoundedRectangle(r, radius);
    speckles(g, r, juce::Colours::white.withAlpha(0.05f), 70, 1.1f, 19);
    speckles(g, r, juce::Colours::black.withAlpha(0.12f), 90, 1.4f, 31);
}

inline void scratches(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour colour, int count, int seed)
{
    juce::Random rng(seed);
    for (int i = 0; i < count; ++i)
    {
        const float x1 = r.getX() + rng.nextFloat() * r.getWidth();
        const float y1 = r.getY() + rng.nextFloat() * r.getHeight();
        const float x2 = x1 + (rng.nextFloat() - 0.5f) * 28.0f;
        const float y2 = y1 + (rng.nextFloat() - 0.35f) * 10.0f;
        g.setColour(colour.withAlpha(0.04f + rng.nextFloat() * 0.08f));
        g.drawLine(x1, y1, x2, y2, 0.8f);
    }
}

inline void screws(juce::Graphics& g, juce::Rectangle<float> r, float inset = 8.0f, float size = 7.0f)
{
    const juce::Point<float> pts[] = {
        { r.getX() + inset, r.getY() + inset },
        { r.getRight() - inset, r.getY() + inset },
        { r.getX() + inset, r.getBottom() - inset },
        { r.getRight() - inset, r.getBottom() - inset }
    };

    for (auto p : pts)
    {
        auto s = juce::Rectangle<float>(size, size).withCentre(p);
        g.setColour(juce::Colours::black.withAlpha(0.35f));
        g.fillEllipse(s.translated(0.0f, 0.8f));
        juce::ColourGradient chrome(juce::Colour(0xffd8d8d8), s.getTopLeft(),
                                    juce::Colour(0xff7a7a7a), s.getBottomRight(), false);
        g.setGradientFill(chrome);
        g.fillEllipse(s);
        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.drawLine(s.getX() + 1.2f, s.getCentreY(), s.getRight() - 1.2f, s.getCentreY(), 1.0f);
        g.drawLine(s.getCentreX(), s.getY() + 1.2f, s.getCentreX(), s.getBottom() - 1.2f, 1.0f);
    }
}

inline void stitch(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour colour, float inset = 5.0f)
{
    g.setColour(colour.withAlpha(0.35f));
    auto inner = r.reduced(inset);
    const float dash = 4.0f, gap = 3.0f;
    auto drawDashed = [&g, dash, gap] (float x1, float y1, float x2, float y2)
    {
        const float dx = x2 - x1, dy = y2 - y1;
        const float len = std::sqrt(dx * dx + dy * dy);
        if (len < 1.0f)
            return;
        const float ux = dx / len, uy = dy / len;
        float d = 0.0f;
        while (d < len)
        {
            const float n = juce::jmin(dash, len - d);
            g.drawLine(x1 + ux * d, y1 + uy * d, x1 + ux * (d + n), y1 + uy * (d + n), 1.0f);
            d += dash + gap;
        }
    };
    drawDashed(inner.getX(), inner.getY(), inner.getRight(), inner.getY());
    drawDashed(inner.getRight(), inner.getY(), inner.getRight(), inner.getBottom());
    drawDashed(inner.getRight(), inner.getBottom(), inner.getX(), inner.getBottom());
    drawDashed(inner.getX(), inner.getBottom(), inner.getX(), inner.getY());
}

inline void diamondPlate(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour colour)
{
    g.saveState();
    juce::Path clip;
    clip.addRoundedRectangle(r, 4.0f);
    g.reduceClipRegion(clip);
    g.setColour(colour);
    const float step = 14.0f;
    for (float y = r.getY() - step; y < r.getBottom() + step; y += step)
    {
        for (float x = r.getX() - step; x < r.getRight() + step; x += step)
        {
            juce::Path d;
            d.addQuadrilateral(x + 3.0f, y + 6.0f, x + 8.0f, y + 3.0f, x + 11.0f, y + 8.0f, x + 6.0f, y + 11.0f);
            g.setColour(colour.brighter(0.18f).withAlpha(0.18f));
            g.fillPath(d);
            g.setColour(colour.darker(0.2f).withAlpha(0.16f));
            g.strokePath(d, juce::PathStrokeType(0.8f));
        }
    }
    g.restoreState();
}

inline void weave(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour a, juce::Colour b, float step = 6.0f)
{
    g.saveState();
    juce::Path clip;
    clip.addRoundedRectangle(r, 4.0f);
    g.reduceClipRegion(clip);
    g.setColour(a);
    for (float x = r.getX(); x < r.getRight(); x += step)
        g.drawLine(x, r.getY(), x, r.getBottom(), 1.0f);
    g.setColour(b);
    for (float y = r.getY(); y < r.getBottom(); y += step)
        g.drawLine(r.getX(), y, r.getRight(), y, 1.0f);
    g.restoreState();
}

inline juce::Font fontSerifItalic(float size)
{
    return juce::Font(juce::FontOptions("Palatino", size, juce::Font::italic));
}

inline juce::Font fontSerifBold(float size)
{
    return juce::Font(juce::FontOptions("Palatino", size, juce::Font::bold));
}

inline juce::Font fontCondensed(float size, bool bold = true)
{
    return juce::Font(juce::FontOptions("Avenir Next Condensed", size, bold ? juce::Font::bold : juce::Font::plain));
}

inline juce::Font fontScript(float size)
{
    return juce::Font(juce::FontOptions("Snell Roundhand", size, juce::Font::plain));
}

inline juce::Font fontDidot(float size)
{
    return juce::Font(juce::FontOptions("Didot", size, juce::Font::bold));
}

inline juce::Font fontHelveticaBold(float size)
{
    return juce::Font(juce::FontOptions("Helvetica Neue", size, juce::Font::bold));
}

inline juce::Font fontImpact(float size)
{
    return juce::Font(juce::FontOptions("Impact", size, juce::Font::plain));
}

inline juce::Font fontCopperplate(float size)
{
    return juce::Font(juce::FontOptions("Copperplate", size, juce::Font::bold));
}

inline juce::Font fontGeorgiaBold(float size)
{
    return juce::Font(juce::FontOptions("Georgia", size, juce::Font::bold));
}

inline juce::Font fontFutura(float size)
{
    return juce::Font(juce::FontOptions("Futura", size, juce::Font::bold));
}

inline juce::Font fontBrush(float size)
{
    return juce::Font(juce::FontOptions("Brush Script MT", size, juce::Font::italic));
}

inline void pinstripes(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour colour, float step, float angleDeg)
{
    g.saveState();
    juce::Path clip;
    clip.addRoundedRectangle(r, 5.0f);
    g.reduceClipRegion(clip);
    g.setColour(colour);
    const float rad = juce::degreesToRadians(angleDeg);
    const float c = std::cos(rad), s = std::sin(rad);
    const float span = r.getWidth() + r.getHeight();
    for (float d = -span; d < span * 2.0f; d += step)
    {
        const float x = r.getX() + d;
        g.drawLine(x, r.getY(), x + s * r.getHeight(), r.getBottom(), 1.6f);
        juce::ignoreUnused(c);
    }
    g.restoreState();
}

inline void waves(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour colour, float amp = 4.0f, float period = 18.0f)
{
    g.saveState();
    juce::Path clip;
    clip.addRoundedRectangle(r, 6.0f);
    g.reduceClipRegion(clip);
    g.setColour(colour);
    for (float y = r.getY() + 6.0f; y < r.getBottom(); y += 9.0f)
    {
        juce::Path p;
        p.startNewSubPath(r.getX(), y);
        for (float x = r.getX(); x <= r.getRight(); x += 3.0f)
            p.lineTo(x, y + std::sin((x - r.getX()) / period * juce::MathConstants<float>::twoPi) * amp);
        g.strokePath(p, juce::PathStrokeType(1.6f));
    }
    g.restoreState();
}

inline void chevrons(juce::Graphics& g, juce::Rectangle<float> r, juce::Colour colour, float step = 10.0f)
{
    g.saveState();
    juce::Path clip;
    clip.addRoundedRectangle(r, 4.0f);
    g.reduceClipRegion(clip);
    g.setColour(colour);
    for (float y = r.getY(); y < r.getBottom() + step; y += step)
    {
        juce::Path p;
        p.startNewSubPath(r.getX(), y);
        p.lineTo(r.getCentreX(), y + step * 0.45f);
        p.lineTo(r.getRight(), y);
        g.strokePath(p, juce::PathStrokeType(1.8f));
    }
    g.restoreState();
}

inline void rings(juce::Graphics& g, juce::Point<float> c, float maxR, juce::Colour colour, int n = 5)
{
    for (int i = 1; i <= n; ++i)
    {
        const float t = (float) i / (float) n;
        g.setColour(colour.withAlpha(colour.getFloatAlpha() * (0.35f + 0.65f * t)));
        g.drawEllipse(c.x - maxR * t,
                      c.y - maxR * t,
                      maxR * 2.0f * t,
                      maxR * 2.0f * t, 1.8f);
    }
}

inline void paintLed(juce::Graphics& g, juce::Rectangle<float> led, bool on)
{
    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.fillEllipse(led.translated(0.8f, 1.4f));
    g.setColour(on ? juce::Colour(theme::kLedOn) : juce::Colour(theme::kLedOff));
    g.fillEllipse(led);
    if (on)
    {
        g.setColour(theme::amber().withAlpha(0.55f));
        g.fillEllipse(led.expanded(2.0f));
        g.setColour(juce::Colours::white.withAlpha(0.55f));
        g.fillEllipse(led.reduced(led.getWidth() * 0.32f).translated(-1.0f, -1.0f));
    }
    else
    {
        g.setColour(juce::Colours::black.withAlpha(0.35f));
        g.drawEllipse(led, 1.0f);
    }
}

inline void paintAmpIcon(juce::Graphics& g, juce::Rectangle<float> b)
{
    auto cab = b.reduced(3.0f);
    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.fillRoundedRectangle(cab.translated(2.0f, 5.0f), 8.0f);
    tolex(g, cab, juce::Colour(0xff2a1812), 8.0f);
    g.setColour(juce::Colour(0xffc9a45a).withAlpha(0.35f));
    g.drawRoundedRectangle(cab.reduced(1.5f), 8.0f, 1.4f);

    auto head = cab.removeFromTop(cab.getHeight() * 0.32f).reduced(6.0f, 5.0f);
    anodized(g, head, juce::Colour(0xffe8d6b0), 4.0f);

    auto cloth = cab.reduced(8.0f, 8.0f);
    g.setColour(juce::Colour(0xff3a2218));
    g.fillRoundedRectangle(cloth, 4.0f);
    weave(g, cloth, juce::Colour(0xffd8c4a0).withAlpha(0.28f), juce::Colour(0xff1a0c0a).withAlpha(0.35f), 4.0f);
    g.setColour(juce::Colour(0xffe8d6b0).withAlpha(0.35f));
    g.drawRoundedRectangle(cloth, 4.0f, 1.2f);
}

inline void paintPedalIcon(juce::Graphics& g, juce::Rectangle<float> b, const juce::String& id)
{
    auto r = b.reduced(1.5f);
    if (id == "tubeScreamer")
    {
        vinyl(g, r, juce::Colour(0xff3aaa3a), 5.0f);
        pinstripes(g, r, juce::Colour(0xff2d8a2d).withAlpha(0.35f), 5.0f, 0.0f);
        auto pad = r.removeFromBottom(r.getHeight() * 0.32f).reduced(4.0f, 3.0f);
        g.setColour(juce::Colour(0xff111111));
        g.fillRoundedRectangle(pad, 3.0f);
        chevrons(g, pad, juce::Colour(0xff2a2a2a).withAlpha(0.7f), 6.0f);
        g.setColour(juce::Colours::white);
        g.setFont(fontCondensed(juce::jmax(8.0f, r.getHeight() * 0.22f)));
        g.drawText("TS", r, juce::Justification::centred);
    }
    else if (id == "bluesbreaker")
    {
        vinyl(g, r, juce::Colour(0xff161412), 5.0f);
        pinstripes(g, r, juce::Colour(0xff2aa4ff).withAlpha(0.18f), 7.0f, 18.0f);
        g.setColour(juce::Colour(0xff2aa4ff));
        g.setFont(fontCondensed(juce::jmax(7.0f, r.getHeight() * 0.18f)));
        g.drawText("BB", r, juce::Justification::centred);
    }
    else if (id == "rat")
    {
        vinyl(g, r, juce::Colour(0xff111111), 4.0f);
        diamondPlate(g, r.reduced(3.0f), juce::Colour(0xff3a3a3a));
        auto badge = r.withSizeKeepingCentre(juce::jmin(r.getWidth() - 8.0f, 36.0f), 14.0f);
        g.setColour(juce::Colours::white);
        g.drawRect(badge, 1.2f);
        g.setFont(fontCondensed(10.0f));
        g.drawText("RAT", badge, juce::Justification::centred);
    }
    else if (id == "shredmaster")
    {
        vinyl(g, r, juce::Colour(0xff101010), 5.0f);
        pinstripes(g, r, juce::Colour(0xffd8c89a).withAlpha(0.16f), 6.0f, 0.0f);
        g.setColour(juce::Colour(0xffd8c89a));
        g.setFont(fontSerifItalic(juce::jmax(8.0f, r.getHeight() * 0.2f)));
        g.drawText("SM", r, juce::Justification::centred);
    }
    else if (id == "chorus")
    {
        vinyl(g, r, juce::Colour(0xff4ec2e8), 5.0f);
        pinstripes(g, r, juce::Colours::white.withAlpha(0.16f), 6.0f, 32.0f);
        g.setColour(juce::Colour(0xff111111));
        g.setFont(fontCondensed(juce::jmax(8.0f, r.getHeight() * 0.2f)));
        g.drawText("CE", r, juce::Justification::centred);
    }
    else if (id == "delay")
    {
        anodized(g, r, juce::Colour(0xff161616), 4.0f);
        rings(g, r.getCentre(), juce::jmin(r.getWidth(), r.getHeight()) * 0.32f, juce::Colours::white.withAlpha(0.22f), 4);
        g.setColour(juce::Colours::white);
        g.setFont(fontCondensed(juce::jmax(7.0f, r.getHeight() * 0.16f)));
        g.drawText("EP", r, juce::Justification::centred);
    }
    else if (id == "slapback")
    {
        anodized(g, r, juce::Colour(0xff1aa7d0), 5.0f);
        waves(g, r, juce::Colours::white.withAlpha(0.22f), 3.0f, 14.0f);
        g.setColour(juce::Colour(0xffeaf7ff));
        g.setFont(fontHelveticaBold(juce::jmax(9.0f, r.getHeight() * 0.18f)));
        g.drawFittedText("AQUA", r.toNearestInt(), juce::Justification::centred, 1);
    }
    else if (id == "reverb")
    {
        anodized(g, r, juce::Colour(0xffc4b08a), 5.0f);
        g.setColour(juce::Colour(0xff2a2418));
        g.setFont(fontDidot(juce::jmax(10.0f, r.getHeight() * 0.16f)));
        g.drawFittedText("GOLDEN", r.reduced(2.0f).toNearestInt(), juce::Justification::centred, 1);
    }
    else
    {
        vinyl(g, r, juce::Colour(0xff333333), 5.0f);
    }
}
} // namespace gfx
