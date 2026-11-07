#include "ToneStack.h"
#include <cmath>

void ToneStack::prepare(double sampleRate)
{
    fs = juce::jmax(1000.0, sampleRate);
    dirty = true;
    reset();
}

void ToneStack::reset()
{
    z1 = z2 = z3 = 0.0;
}

void ToneStack::setTones(float treble, float mid, float bass) noexcept
{
    t = juce::jlimit(0.0005f, 0.9995f, treble);
    m = juce::jlimit(0.0005f, 0.9995f, mid);
    l = juce::jlimit(0.0005f, 0.9995f, bass);
    dirty = true;
}

void ToneStack::recompute()
{
    const double c1 = parts.c1, c2 = parts.c2, c3 = parts.c3;
    const double r1 = parts.r1, r2 = parts.r2, r3 = parts.r3, r4 = parts.r4;
    const double T = (double) t, M = (double) m, L = (double) l;

    const double nb1 = T * c1 * r1 + M * c3 * r3 + L * (c1 * r2 + c2 * r2) + r2 * c1 + r3 * (c1 + c2);
    const double nb2 = T * (c1 * c2 * r1 * r4 + c1 * c3 * r1 * r4)
                     - M * M * (c1 * c3 * r3 * r3 + c2 * c3 * r3 * r3)
                     + M * (c1 * c3 * r1 * r3 + c1 * c3 * r3 * r3 + c2 * c3 * r3 * r3)
                     + L * (c1 * c2 * r1 * r2 + c1 * c2 * r2 * r4 + c1 * c3 * r2 * r4)
                     + L * M * (c1 * c3 * r2 * r3 + c2 * c3 * r2 * r3)
                     + r1 * c1 * c2 * r4 + r1 * c1 * c3 * r4 + r3 * (c1 + c2) * c3 * r4;
    const double nb3 = L * M * (c1 * c2 * c3 * r1 * r2 * r3 + c1 * c2 * c3 * r2 * r3 * r4)
                     - M * M * (c1 * c2 * c3 * r1 * r3 * r3 + c1 * c2 * c3 * r3 * r3 * r4)
                     + M * (c1 * c2 * c3 * r1 * r3 * r4)
                     + T * c1 * c2 * c3 * r1 * r3 * r4;

    const double na0 = 1.0;
    const double na1 = (c1 * r1 + c1 * r3 + c2 * r3 + c2 * r4 + c3 * r4)
                     + M * c3 * r3 + L * (c1 * r2 + c2 * r2);
    const double na2 = M * (c1 * c3 * r1 * r3 - c2 * c3 * r3 * r3 + c1 * c3 * r3 * r3 + c2 * c3 * r3 * r4)
                     + L * M * (c1 * c3 * r2 * r3 + c2 * c3 * r2 * r3)
                     + L * (c1 * c2 * r2 * r4 + c1 * c2 * r1 * r2 + c1 * c3 * r2 * r4 + c2 * c3 * r2 * r4)
                     + (c1 * c2 * r1 * r4 + c1 * c3 * r1 * r4 + c1 * c2 * r3 * r4 + c1 * c3 * r3 * r4 + c2 * c3 * r3 * r4);
    const double na3 = L * M * (c1 * c2 * c3 * r1 * r2 * r3 + c1 * c2 * c3 * r2 * r3 * r4)
                     - M * M * (c1 * c2 * c3 * r3 * r3 * r4 + c1 * c2 * c3 * r1 * r3 * r3)
                     + M * c1 * c2 * c3 * r1 * r3 * r4
                     + c1 * c2 * c3 * r1 * r3 * r4;

    const double c = 2.0 * fs;
    const double c2s = c * c;
    const double c3s = c2s * c;

    const double B0 =  nb1 * c + nb2 * c2s + nb3 * c3s;
    const double B1 =  nb1 * c - nb2 * c2s - 3.0 * nb3 * c3s;
    const double B2 = -nb1 * c - nb2 * c2s + 3.0 * nb3 * c3s;
    const double B3 = -nb1 * c + nb2 * c2s - nb3 * c3s;
    const double A0 =  na0 + na1 * c + na2 * c2s + na3 * c3s;
    const double A1 =  3.0 * na0 + na1 * c - na2 * c2s - 3.0 * na3 * c3s;
    const double A2 =  3.0 * na0 - na1 * c - na2 * c2s + 3.0 * na3 * c3s;
    const double A3 =  na0 - na1 * c + na2 * c2s - na3 * c3s;

    const double inv = (std::abs(A0) > 1.0e-30) ? (1.0 / A0) : 1.0;
    b0 = B0 * inv;
    b1 = B1 * inv;
    b2 = B2 * inv;
    b3 = B3 * inv;
    a1 = A1 * inv;
    a2 = A2 * inv;
    a3 = A3 * inv;

    if (topology == Topology::Marshall)
    {
        b0 *= 1.15;
        b1 *= 1.15;
        b2 *= 1.15;
        b3 *= 1.15;
    }

    if (! std::isfinite(b0) || ! std::isfinite(a1) || ! std::isfinite(a2) || ! std::isfinite(a3))
    {
        b0 = 1.0;
        b1 = b2 = b3 = 0.0;
        a1 = a2 = a3 = 0.0;
    }

    dirty = false;
}

float ToneStack::process(float x) noexcept
{
    if (dirty)
        recompute();

    const double y = b0 * (double) x + z1;
    z1 = b1 * (double) x - a1 * y + z2;
    z2 = b2 * (double) x - a2 * y + z3;
    z3 = b3 * (double) x - a3 * y;

    if (! std::isfinite(y))
    {
        z1 = z2 = z3 = 0.0;
        return 0.0f;
    }
    return (float) y;
}
