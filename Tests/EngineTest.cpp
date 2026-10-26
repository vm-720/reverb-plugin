// Offline sanity checks for the reverb engine: loudness, measured RT60, stability, freeze.
#include "../Source/ReverbEngine.h"

#include <cstdio>
#include <random>

static bool allFinite (const std::vector<float>& v)
{
    for (auto x : v)
        if (! std::isfinite (x))
            return false;
    return true;
}

static double rms (const std::vector<float>& v, size_t from, size_t to)
{
    double sum = 0;
    for (size_t i = from; i < to; ++i)
        sum += (double) v[i] * v[i];
    return std::sqrt (sum / (double) (to - from));
}

int main()
{
    const double sr = 48000.0;
    int failures = 0;

    for (float size : { 0.0f, 0.5f, 1.0f })
        for (float decay : { 0.5f, 2.5f, 10.0f })
        {
            reverb::Engine engine;
            reverb::Parameters p;
            p.size = size;
            p.decaySeconds = decay;
            p.damping = 0.0f;
            p.lowCutHz = 20.0f;
            p.highCutHz = 20000.0f;
            engine.prepare (sr);
            engine.setParameters (p);

            // 1 s of noise at -20 dBFS, then silence; measure the tail.
            const size_t total = (size_t) (sr * (1.0 + decay * 1.5));
            std::vector<float> l (total, 0.0f), r (total, 0.0f);
            std::mt19937 gen (1);
            std::normal_distribution<float> noise (0.0f, 0.1f);
            for (size_t i = 0; i < (size_t) sr; ++i)
                l[i] = r[i] = noise (gen);

            for (size_t i = 0; i < total; i += 512)
                engine.process (l.data() + i, r.data() + i, (int) std::min<size_t> (512, total - i));

            const double steady = rms (l, (size_t) (sr * 0.7), (size_t) sr);
            // Energy decay between 0.1*decay and 0.6*decay after the input stops gives -30 dB slope.
            const double start = std::fmax (decay * 0.1, 0.4); // skip input still in flight in long delays
            const size_t a = (size_t) (sr * (1.0 + start)), b = (size_t) (sr * (1.0 + start + decay * 0.5));
            const size_t w = (size_t) (sr * 0.05);
            const double ea = rms (l, a, a + w), eb = rms (l, b, b + w);
            const double dbPerSec = 20.0 * std::log10 (eb / ea) / (decay * 0.5);
            const double measuredRt60 = -60.0 / dbPerSec;

            const bool ok = allFinite (l) && allFinite (r) && std::fabs (measuredRt60 / decay - 1.0) < 0.35;
            failures += ok ? 0 : 1;
            std::printf ("size %.1f decay %5.1fs  wet/dry %+5.1f dB  RT60 %6.2fs  %s\n", size, decay,
                         20.0 * std::log10 (steady / 0.1), measuredRt60, ok ? "ok" : "FAIL");
        }

    {
        // Freeze must hold its level without blowing up.
        reverb::Engine engine;
        reverb::Parameters p;
        engine.prepare (sr);
        engine.setParameters (p);
        std::vector<float> l ((size_t) sr * 12, 0.0f), r ((size_t) sr * 12, 0.0f);
        std::mt19937 gen (2);
        std::normal_distribution<float> noise (0.0f, 0.1f);
        for (size_t i = 0; i < (size_t) sr; ++i)
            l[i] = r[i] = noise (gen);
        for (size_t i = 0; i < l.size(); i += 512)
        {
            if (i >= (size_t) sr && ! p.freeze)
            {
                p.freeze = true;
                engine.setParameters (p);
            }
            engine.process (l.data() + i, r.data() + i, 512);
        }
        const double early = rms (l, (size_t) (sr * 2), (size_t) (sr * 3));
        const double late = rms (l, (size_t) (sr * 11), (size_t) (sr * 12));
        const double driftDb = 20.0 * std::log10 (late / early);
        const bool ok = allFinite (l) && std::fabs (driftDb) < 6.0;
        failures += ok ? 0 : 1;
        std::printf ("freeze drift over 9 s: %+.1f dB  %s\n", driftDb, ok ? "ok" : "FAIL");
    }

    std::printf (failures == 0 ? "ALL PASSED\n" : "%d FAILED\n", failures);
    return failures == 0 ? 0 : 1;
}
