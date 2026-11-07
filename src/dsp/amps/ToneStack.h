#pragma once

#include <JuceHeader.h>

class ToneStack
{
public:
    enum class Topology { Fender, Marshall };

    struct Parts
    {
        double c1 = 250e-12;
        double c2 = 20e-9;
        double c3 = 20e-9;
        double r1 = 250e3;
        double r2 = 1e6;
        double r3 = 25e3;
        double r4 = 56e3;
    };

    static Parts deluxeAb763() noexcept
    {
        return { 250e-12, 100e-9, 47e-9, 250e3, 250e3, 10e3, 100e3 };
    }

    static Parts steelString() noexcept
    {
        return { 250e-12, 47e-9, 47e-9, 250e3, 250e3, 25e3, 56e3 };
    }

    static Parts dualRect() noexcept
    {
        return { 250e-12, 22e-9, 22e-9, 250e3, 1e6, 25e3, 33e3 };
    }

    void prepare(double sampleRate);
    void reset();
    void setTopology(Topology t) noexcept { topology = t; dirty = true; }
    void setParts(const Parts& p) noexcept { parts = p; dirty = true; }
    void setTones(float treble, float mid, float bass) noexcept;
    float process(float x) noexcept;

private:
    void recompute();

    Topology topology = Topology::Fender;
    Parts parts;
    double fs = 44100.0;
    float t = 0.5f, m = 0.5f, l = 0.5f;
    bool dirty = true;
    double b0 = 1, b1 = 0, b2 = 0, b3 = 0;
    double a1 = 0, a2 = 0, a3 = 0;
    double z1 = 0, z2 = 0, z3 = 0;
};
