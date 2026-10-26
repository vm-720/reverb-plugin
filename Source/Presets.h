#pragma once

#include <array>
#include <initializer_list>
#include <utility>

// Factory presets: a name plus the plain (un-normalised) value of every parameter it sets.
// Parameters a preset doesn't mention are reset to their defaults.
struct FactoryPreset
{
    const char* name;
    std::initializer_list<std::pair<const char*, float>> values;
};

inline const std::array<FactoryPreset, 8>& getFactoryPresets()
{
    static const std::array<FactoryPreset, 8> presets { {
        { "Default", {} },
        { "Small Room", { { "size", 0.15f }, { "decay", 0.6f }, { "predelay", 5.0f }, { "damping", 0.55f },
                          { "density", 1.0f }, { "modulation", 0.1f }, { "mix", 0.25f } } },
        { "Vocal Plate", { { "size", 0.35f }, { "decay", 1.8f }, { "predelay", 40.0f }, { "damping", 0.2f },
                           { "density", 1.0f }, { "lowcut", 250.0f }, { "highcut", 14000.0f },
                           { "ducking", 0.35f }, { "mix", 0.25f } } },
        { "Big Hall", { { "size", 0.75f }, { "decay", 3.8f }, { "predelay", 30.0f }, { "damping", 0.45f },
                        { "density", 0.9f }, { "modulation", 0.35f }, { "mix", 0.3f } } },
        { "Cathedral", { { "size", 1.0f }, { "decay", 8.0f }, { "predelay", 60.0f }, { "damping", 0.5f },
                         { "density", 0.85f }, { "modulation", 0.4f }, { "lowcut", 80.0f },
                         { "highcut", 9000.0f }, { "mix", 0.35f } } },
        { "Ambient Wash", { { "size", 0.9f }, { "decay", 15.0f }, { "predelay", 0.0f }, { "damping", 0.35f },
                            { "density", 0.7f }, { "modulation", 0.8f }, { "modrate", 0.3f },
                            { "lowcut", 200.0f }, { "highcut", 7000.0f }, { "mix", 0.5f } } },
        { "Echo Space", { { "size", 0.6f }, { "decay", 4.0f }, { "predelay", 120.0f }, { "damping", 0.4f },
                          { "density", 0.0f }, { "modulation", 0.2f }, { "ducking", 0.5f }, { "mix", 0.3f } } },
        { "Frozen Pad", { { "size", 0.8f }, { "decay", 20.0f }, { "density", 1.0f }, { "modulation", 0.6f },
                          { "modrate", 0.2f }, { "freeze", 1.0f }, { "mix", 0.6f } } },
    } };
    return presets;
}
