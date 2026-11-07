#pragma once

#include <JuceHeader.h>

class PowerAmp
{
public:
    void prepare(double sampleRate);
    void reset();
    void setDrive(float linear) noexcept { drive = linear; }
    void setSag(float amount, float attackMs, float releaseMs) noexcept;
    void setPresence(float zeroToOne) noexcept;
    void setResonance(float zeroToOne) noexcept;
    void enablePresence(bool on) noexcept { usePresence = on; }
    float process(float x) noexcept;

private:
    void updatePresence();

    float sr = 44100.0f;
    float drive = 1.2f;
    float sagAmt = 0.25f;
    float env = 0.0f;
    float atk = 0.1f;
    float rel = 0.01f;
    float presenceN = 0.5f;
    float resonanceN = 0.35f;
    bool usePresence = false;
    bool presDirty = true;
    float hpA = 0.0f, hpZ = 0.0f, hpX = 0.0f;
    float lpA = 0.0f, lpZ = 0.0f;
};
