# Simple Reverb

A starter algorithmic reverb plugin (VST3 + standalone) built with JUCE and CMake.

Controls: Size, Damping, Width, Pre-Delay (0-500 ms), Mix, Freeze.

The DSP is JUCE's `dsp::Reverb` (a Freeverb-style comb/allpass network) with a smoothed
pre-delay line on the wet path and an equal-power dry/wet mixer. It's a placeholder engine
meant to be swapped for something better (FDN, plate, etc.) later.

## Requirements (Windows)

- Visual Studio 2022 or Build Tools 2022 with the "Desktop development with C++" workload
- CMake 3.22+
- Git (CMake downloads JUCE from GitHub on first configure)

## Build

```
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

Output: `build/SimpleReverb_artefacts/Release/VST3/Simple Reverb.vst3`

## Install

Copy `Simple Reverb.vst3` into `C:\Program Files\Common Files\VST3` (needs admin), then rescan
plugins in your DAW.
