#pragma once

// Stereo algorithmic reverb: input diffuser (4 stages of delay + Hadamard mixing) feeding an
// 8-channel feedback delay network with Householder feedback, per-channel damping and
// slow delay-time modulation. Plain C++ with no JUCE dependency so it can be tested offline.

#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace reverb
{

constexpr int numChannels = 8;
using Frame = std::array<float, numChannels>;
constexpr double twoPi = 6.283185307179586;

class DelayLine
{
public:
    void prepare (int maxDelaySamples)
    {
        int size = 1;
        while (size < maxDelaySamples + 4)
            size <<= 1;
        buffer.assign ((size_t) size, 0.0f);
        mask = size - 1;
        writePos = 0;
    }

    void clear() { std::fill (buffer.begin(), buffer.end(), 0.0f); }

    void write (float x)
    {
        buffer[(size_t) writePos] = x;
        writePos = (writePos + 1) & mask;
    }

    // Delay is measured from the most recently written sample, so call after write().
    float read (float delaySamples) const
    {
        const float pos = (float) writePos - 1.0f - delaySamples;
        const float floorPos = std::floor (pos);
        const int i0 = (int) floorPos;
        const float frac = pos - floorPos;
        const float a = buffer[(size_t) (i0 & mask)];
        const float b = buffer[(size_t) ((i0 + 1) & mask)];
        return a + frac * (b - a);
    }

private:
    std::vector<float> buffer;
    int mask = 0;
    int writePos = 0;
};

inline void hadamardInPlace (Frame& x)
{
    for (int h = 1; h < numChannels; h <<= 1)
        for (int i = 0; i < numChannels; i += h * 2)
            for (int j = i; j < i + h; ++j)
            {
                const float a = x[(size_t) j], b = x[(size_t) (j + h)];
                x[(size_t) j] = a + b;
                x[(size_t) (j + h)] = a - b;
            }

    const float scale = 1.0f / std::sqrt ((float) numChannels);
    for (auto& v : x)
        v *= scale;
}

inline void householderInPlace (Frame& x)
{
    float sum = 0.0f;
    for (auto v : x)
        sum += v;
    sum *= 2.0f / (float) numChannels;
    for (auto& v : x)
        v -= sum;
}

// Small deterministic RNG so every instance (and every session) sounds identical.
struct Rng
{
    uint32_t state;
    float next() // [0, 1)
    {
        state = state * 1664525u + 1013904223u;
        return (float) (state >> 8) / 16777216.0f;
    }
};

class DiffuserStage
{
public:
    void prepare (double sampleRate, float maxRangeMs, uint32_t seed)
    {
        Rng rng { seed };
        const float maxSamples = maxRangeMs * 0.001f * (float) sampleRate;
        for (int i = 0; i < numChannels; ++i)
        {
            // Spread delays evenly across the range, with jitter inside each slot.
            const float slotLow = (float) i / numChannels;
            const float slotHigh = (float) (i + 1) / numChannels;
            baseDelay[(size_t) i] = maxSamples * (slotLow + rng.next() * (slotHigh - slotLow));
            flip[(size_t) i] = rng.next() > 0.5f;
            lines[(size_t) i].prepare ((int) std::ceil (maxSamples) + 2);
        }

        // Fisher-Yates shuffle of the channel order.
        for (int i = 0; i < numChannels; ++i)
            order[(size_t) i] = i;
        for (int i = numChannels - 1; i > 0; --i)
            std::swap (order[(size_t) i], order[(size_t) (int) (rng.next() * (float) (i + 1))]);
    }

    void clear()
    {
        for (auto& l : lines)
            l.clear();
    }

    void process (Frame& x, float sizeScale)
    {
        Frame delayed;
        for (int i = 0; i < numChannels; ++i)
        {
            lines[(size_t) i].write (x[(size_t) i]);
            delayed[(size_t) i] = lines[(size_t) i].read (baseDelay[(size_t) i] * sizeScale);
        }

        for (int i = 0; i < numChannels; ++i)
        {
            const float v = delayed[(size_t) order[(size_t) i]];
            x[(size_t) i] = flip[(size_t) i] ? -v : v;
        }

        hadamardInPlace (x);
    }

private:
    std::array<DelayLine, numChannels> lines;
    std::array<float, numChannels> baseDelay {};
    std::array<bool, numChannels> flip {};
    std::array<int, numChannels> order {};
};

// RBJ biquad, used for the wet low/high cut.
class Biquad
{
public:
    void setHighPass (double sampleRate, double freq) { design (sampleRate, freq, true); }
    void setLowPass (double sampleRate, double freq) { design (sampleRate, freq, false); }
    void clear() { z1 = z2 = 0.0f; }

    float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }

private:
    void design (double sampleRate, double freq, bool highPass)
    {
        freq = std::fmin (freq, sampleRate * 0.45);
        const double w = twoPi * freq / sampleRate;
        const double cosw = std::cos (w);
        const double alpha = std::sin (w) / (2.0 * 0.7071067811865476);
        const double a0 = 1.0 + alpha;
        const double k = highPass ? (1.0 + cosw) * 0.5 : (1.0 - cosw) * 0.5;

        b0 = (float) (k / a0);
        b1 = (float) ((highPass ? -2.0 * k : 2.0 * k) / a0);
        b2 = b0;
        a1 = (float) (-2.0 * cosw / a0);
        a2 = (float) ((1.0 - alpha) / a0);
    }

    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1 = 0, z2 = 0;
};

struct Parameters
{
    float size = 0.5f;        // 0..1, scales every delay time
    float decaySeconds = 2.5f; // RT60 of the tail
    float damping = 0.4f;     // 0..1, how fast highs die away in the tail
    float lowCutHz = 100.0f;
    float highCutHz = 12000.0f;
    float modulation = 0.3f;  // 0..1, depth of delay-time wobble
    float modRateHz = 0.5f;   // average LFO rate; each line runs at a different multiple
    float density = 0.8f;     // 0..1, how many diffuser stages smear the input (0 = echoey, 1 = smooth)
    float width = 1.0f;       // 0 = mono, 1 = full stereo
    bool freeze = false;
};

class Engine
{
public:
    static constexpr float maxFdnMs = 300.0f;
    static constexpr float fdnBaseMs = 150.0f;
    static constexpr float maxModMs = 1.2f;

    void prepare (double newSampleRate)
    {
        sampleRate = newSampleRate;

        static constexpr std::array<float, 4> diffuserRangesMs { 45.0f, 23.0f, 11.0f, 6.0f };
        for (size_t s = 0; s < diffusers.size(); ++s)
            diffusers[s].prepare (sampleRate, diffuserRangesMs[s], 0x9e3779b9u + (uint32_t) s * 7919u);

        // FDN delays spread exponentially over one octave so their modes don't line up.
        Rng rng { 0x2545f491u };
        const int maxSamples = (int) std::ceil ((maxFdnMs + maxModMs) * 0.001 * sampleRate) + 4;
        for (int i = 0; i < numChannels; ++i)
        {
            const float r = ((float) i + rng.next() * 0.8f) / numChannels;
            fdnBase[(size_t) i] = fdnBaseMs * 0.001f * (float) sampleRate * std::pow (2.0f, r);
            fdnLines[(size_t) i].prepare (maxSamples);
            lfoPhase[(size_t) i] = rng.next();
            lfoRate[(size_t) i] = 0.6f + 0.8f * rng.next(); // multiple of modRateHz
        }

        smoothCoeff = 1.0f - (float) std::exp (-1.0 / (0.08 * sampleRate)); // ~80 ms glide
        sizeScaleSmoothed = targetSizeScale;
        reset();
        setParameters (params);
        sizeScaleSmoothed = targetSizeScale;
    }

    void reset()
    {
        for (auto& d : diffusers)
            d.clear();
        for (auto& l : fdnLines)
            l.clear();
        dampState.fill (0.0f);
        for (auto& f : lowCut)
            f.clear();
        for (auto& f : highCut)
            f.clear();
    }

    void setParameters (const Parameters& p)
    {
        params = p;
        targetSizeScale = 0.12f + 0.88f * std::pow (p.size, 1.3f);

        // Damping maps to a loop low-pass from 18 kHz down to 1.2 kHz (log scale).
        const double dampHz = 18000.0 * std::pow (1200.0 / 18000.0, (double) p.damping);
        dampCoeff = p.freeze ? 1.0f : 1.0f - (float) std::exp (-twoPi * dampHz / sampleRate);

        for (auto& f : lowCut)
            f.setHighPass (sampleRate, p.lowCutHz);
        for (auto& f : highCut)
            f.setLowPass (sampleRate, p.highCutHz);

        modDepthSamples = p.modulation * maxModMs * 0.001f * (float) sampleRate;
        updateFeedbackGains (sizeScaleSmoothed);
    }

    // In-place stereo processing; the output is 100% wet.
    void process (float* left, float* right, int numSamples)
    {
        updateFeedbackGains (sizeScaleSmoothed);

        const float inputGain = params.freeze ? 0.0f : 1.0f;
        const float lfoInc = params.modRateHz / (float) sampleRate;
        for (size_t s = 0; s < stageWeight.size(); ++s)
            stageWeight[s] = std::fmin (std::fmax (params.density * 4.0f - (float) s, 0.0f), 1.0f);
        const float mid = 0.5f, side = 0.5f * params.width;

        for (int n = 0; n < numSamples; ++n)
        {
            sizeScaleSmoothed += smoothCoeff * (targetSizeScale - sizeScaleSmoothed);

            Frame x;
            const float inL = left[n] * inputGain, inR = right[n] * inputGain;
            for (int i = 0; i < numChannels; ++i)
                x[(size_t) i] = (i & 1) ? inR : inL;

            // Density fades the diffuser stages in one after another.
            for (size_t s = 0; s < diffusers.size(); ++s)
            {
                const Frame dry = x;
                diffusers[s].process (x, sizeScaleSmoothed);
                const float w = stageWeight[s];
                for (int i = 0; i < numChannels; ++i)
                    x[(size_t) i] = dry[(size_t) i] + w * (x[(size_t) i] - dry[(size_t) i]);
            }

            // Read the FDN outputs (modulated), damp them and form the stereo output.
            Frame fb;
            float outL = 0.0f, outR = 0.0f;
            for (int i = 0; i < numChannels; ++i)
            {
                auto& phase = lfoPhase[(size_t) i];
                phase += lfoRate[(size_t) i] * lfoInc;
                if (phase >= 1.0f)
                    phase -= 1.0f;
                const float mod = modDepthSamples * (0.5f + 0.5f * fastSin (phase));
                const float delay = fdnBase[(size_t) i] * sizeScaleSmoothed + mod;

                const float y = fdnLines[(size_t) i].read (delay);
                auto& s = dampState[(size_t) i];
                s += dampCoeff * (y - s);
                fb[(size_t) i] = s * feedbackGain[(size_t) i];

                if (i & 1)
                    outR += y;
                else
                    outL += y;
            }

            householderInPlace (fb);
            for (int i = 0; i < numChannels; ++i)
                fdnLines[(size_t) i].write (fb[(size_t) i] + x[(size_t) i]);

            outL *= outputGain;
            outR *= outputGain;

            const float m = mid * (outL + outR), sd = side * (outL - outR);
            float l = m + sd, r = m - sd;

            l = highCut[0].process (lowCut[0].process (l));
            r = highCut[1].process (lowCut[1].process (r));

            left[n] = l;
            right[n] = r;
        }
    }

private:
    void updateFeedbackGains (float sizeScale)
    {
        for (int i = 0; i < numChannels; ++i)
        {
            if (params.freeze)
            {
                feedbackGain[(size_t) i] = 1.0f;
                continue;
            }

            // Per-channel gain so every delay loses 60 dB in exactly decaySeconds.
            const double delaySeconds = fdnBase[(size_t) i] * sizeScale / sampleRate;
            feedbackGain[(size_t) i] = (float) std::pow (10.0, -3.0 * delaySeconds / (double) params.decaySeconds);
        }
    }

    // Parabolic sine approximation, phase in [0, 1).
    static float fastSin (float phase)
    {
        const float x = phase * 2.0f - 1.0f; // -1..1 maps to -pi..pi
        return -4.0f * x * (1.0f - std::fabs (x));
    }

    double sampleRate = 48000.0;
    Parameters params;

    std::array<DiffuserStage, 4> diffusers;
    std::array<DelayLine, numChannels> fdnLines;
    std::array<float, numChannels> fdnBase {};
    std::array<float, numChannels> feedbackGain {};
    std::array<float, numChannels> lfoPhase {};
    std::array<float, numChannels> lfoRate {};
    std::array<float, 4> stageWeight { 1.0f, 1.0f, 1.0f, 1.0f };
    Frame dampState {};

    std::array<Biquad, 2> lowCut, highCut;

    float targetSizeScale = 0.5f;
    float sizeScaleSmoothed = 0.5f;
    float smoothCoeff = 0.001f;
    float dampCoeff = 1.0f;
    float modDepthSamples = 0.0f;
    float outputGain = 0.5f;
};

} // namespace reverb
