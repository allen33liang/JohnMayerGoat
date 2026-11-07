#pragma once

#include <JuceHeader.h>

namespace theme
{
inline constexpr juce::uint32 kStudioTop    = 0xff2c241c;
inline constexpr juce::uint32 kStudioBottom = 0xff0e0c0a;
inline constexpr juce::uint32 kWood         = 0xff3a2a1c;
inline constexpr juce::uint32 kIvory        = 0xfff3e6c4;
inline constexpr juce::uint32 kAmber        = 0xffffb24a;
inline constexpr juce::uint32 kLedOn        = 0xffff8a2a;
inline constexpr juce::uint32 kLedOff       = 0xff3a1a10;
inline constexpr juce::uint32 kTolex        = 0xff2a1812;
inline constexpr juce::uint32 kCreamPlate   = 0xffe8d6b0;
inline constexpr juce::uint32 kChromeHi     = 0xfff0ece4;
inline constexpr juce::uint32 kChromeLo     = 0xff8a8478;

inline juce::Colour ivory()     { return juce::Colour(kIvory); }
inline juce::Colour amber()     { return juce::Colour(kAmber); }
inline juce::Colour cream()     { return juce::Colour(kCreamPlate); }

inline void dropShadow(juce::Graphics& g, juce::Rectangle<float> r, float radius, float depth = 6.0f)
{
    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.fillRoundedRectangle(r.translated(2.0f, depth), radius);
}

inline void topLeftSheen(juce::Graphics& g, juce::Rectangle<float> r, float radius)
{
    juce::ColourGradient sheen(juce::Colours::white.withAlpha(0.16f), r.getTopLeft(),
                               juce::Colours::transparentBlack, r.getBottomRight(), false);
    g.setGradientFill(sheen);
    g.fillRoundedRectangle(r, radius);
}

inline void vignette(juce::Graphics& g, juce::Rectangle<float> r)
{
    juce::ColourGradient v(juce::Colours::transparentBlack, r.getCentre(),
                           juce::Colours::black.withAlpha(0.55f), r.getBottomLeft(), true);
    g.setGradientFill(v);
    g.fillRect(r);
}

inline void studioFloor(juce::Graphics& g, juce::Rectangle<float> r)
{
    juce::ColourGradient floor(juce::Colour(kStudioTop), r.getTopLeft(),
                               juce::Colour(kStudioBottom), r.getBottomRight(), false);
    g.setGradientFill(floor);
    g.fillRect(r);

    juce::Random rng(41);
    for (int i = 0; i < 90; ++i)
    {
        const float x = r.getX() + rng.nextFloat() * r.getWidth();
        const float y = r.getY() + rng.nextFloat() * r.getHeight();
        g.setColour(juce::Colour(kWood).withAlpha(0.08f + rng.nextFloat() * 0.06f));
        g.fillRoundedRectangle(x, y, 18.0f + rng.nextFloat() * 40.0f, 2.0f, 1.0f);
    }
    vignette(g, r);
}

inline void ticks(juce::Graphics& g, juce::Point<float> c, float inner, float outer,
                  float start, float end, int count, juce::Colour colour)
{
    g.setColour(colour);
    for (int i = 0; i < count; ++i)
    {
        const float t = (float) i / (float) (count - 1);
        const float a = start + t * (end - start);
        const float x0 = c.x + std::cos(a) * inner;
        const float y0 = c.y + std::sin(a) * inner;
        const float x1 = c.x + std::cos(a) * outer;
        const float y1 = c.y + std::sin(a) * outer;
        g.drawLine(x0, y0, x1, y1, i == 0 || i == count - 1 || i == count / 2 ? 1.8f : 1.1f);
    }
}
} // namespace theme
