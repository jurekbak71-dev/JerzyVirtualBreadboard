# Microphonic Fuzz VST3

Experimental Windows x64 VST3 audio effect for FL Studio. This repository replaces the retired Virtual Breadboard project; the previous state is preserved in branch `archive/virtual-breadboard-2026-11-13`.

## Current implementation
Initial JUCE/C++20 source scaffold: stereo fuzz with a bounded, one-sample feedback path, input/output gains, wet/dry mix, and a basic resizable editor. This is **not** the approved full Virtual Mic + room IR engine. No plugin build or DAW test has been performed.

## Build (planned)
Requirements: CMake 3.22+, C++20 compiler, JUCE 8 fetched via CMake, Windows x64 toolchain.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release
```

JUCE is pinned to a version tag in CMakeLists.txt; dependency availability and build have not been verified.

## Planned DSP
Virtual Mic, convolution room IR *inside* feedback path, modes A (user IR only) / B (factory + user IR), filtering, tremolo/flanger, limiter and photorealistic modular GUI.

## Assets
GUI prototype v0.1 was generated separately (151 PNG/JSON/README files, including a 128-frame knob filmstrip). Binary assets are **not yet committed to this repository**. Do not assume `assets/gui/` contains the artwork.

## Status
Source scaffold only; no compilation, automated tests, or FL Studio verification yet.
