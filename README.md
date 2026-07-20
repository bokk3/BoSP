# BoDSP

> **A collection of professional VST3 audio plugins and a shared C++20 DSP framework — built with JUCE and CMake.**

---

## Plugins

| Plugin | Description | Key DSP |
|--------|-------------|---------|
| 🔥 **Distortion** | Waveshaper saturation — Soft / Tube / Hard / Fold modes | `WaveShaper`, `Tone`, `DryWet`, `SoftClipper` |
| 🌊 **Delay** | Multitap stereo delay with auto-ducking, LP/HP filtering | `Delay`, `DelayLine`, `EnvelopeFollower` |
| 🌌 **Reverb** | Freeverb-based stereo reverb with pre-delay and tone shaping | `Reverb`, `OnePoleFilters` |
| 🎛️ **Filter** | Biquad filter — Lowpass, Highpass, Low/High Shelf, Notch | `BiquadFilter`, `DryWet` |
| 🌀 **Chorus** | 4-voice stereo modulated delay with spread and feedback | `LFO`, `DelayLine`, `DryWet`, `ParameterSmoother` |

All plugins ship as **VST3 + Standalone** and share a unified Hard Techno GUI:
dark industrial gradients, neon orange-red rotary knobs, per-plugin VU meters, and a soft-clipper safety toggle.

---

## Shared DSP Framework (`shared/DSP/`)

The framework is the backbone. All modules are **C++20, JUCE-free, header-only, and realtime-safe** — no heap allocations inside `process()`.

### Foundation Libraries

| File | Description |
|------|-------------|
| `Gain.h` | Linear gain stage |
| `WaveShaper.h` | Soft / Tube / Hard / Fold waveshaping |
| `Tone.h` | One-pole LP tone control |
| `DelayLine.h` | Interpolated circular buffer |
| `Delay.h` | Full delay with LP/HP filters and auto-duck |
| `Reverb.h` | Freeverb-style stereo reverb |
| `BiquadFilter.h` | Multi-mode biquad filter |
| `OnePoleFilters.h` | Lightweight one-pole LP/HP |

### New DSP Modules

| File | Description |
|------|-------------|
| `DryWet.h` | Equal-power or linear dry/wet blend |
| `ParameterSmoother.h` | Linear ramp or exponential parameter smoothing |
| `EnvelopeFollower.h` | Peak/RMS ballistics (attack/release) |
| `LFO.h` | Sine / Triangle / Saw / Square / S&H |
| `DCBlocker.h` | One-pole DC removal, multi-channel |
| `SoftClipper.h` | Tanh / Atan / Cubic / SoftKnee saturation |
| `NoiseGenerator.h` | White and pink noise (Kellett approximation) |
| `Meter.h` | Peak, windowed RMS, peak hold, clip detect |
| `StereoTools.h` | Width, mono, swap, M/S encode/decode |
| `Oversampler.h` | 2x/4x oversampling with FIR extension points |

See [`progress-dsp.md`](progress-dsp.md) for the full status, API cheatsheet, and roadmap.

---

## Quick Start (Windows / MSVC)

### Prerequisites
- **Visual Studio** with C++ workload (MSVC 2022 recommended)
- **CMake** 3.22+
- **JUCE** cloned or extracted into `external/JUCE`

### 1 — Configure
```powershell
cmake -S . -B build
```
> For Ninja: `cmake -S . -B build -G "Ninja"`

### 2 — Build (Debug)
```powershell
cmake --build build --config Debug
```

### 3 — Build (Release)
```powershell
cmake --build build --config Release
```

### Build a single plugin
```powershell
cmake --build build --config Release --target BoDSPChorus_VST3
```

### Output locations
```
build/plugins/<PluginName>/<PluginName>_artefacts/<Config>/VST3/
```

---

## Project Layout

```
BoSP/
├── shared/
│   └── DSP/                  Reusable C++20 DSP headers (JUCE-free)
├── plugins/
│   ├── Distortion/           Waveshaper saturation plugin
│   ├── Delay/                Stereo delay with ducking
│   ├── Reverb/               Freeverb reverb plugin
│   ├── Filter/               Biquad filter plugin
│   └── Chorus/               4-voice stereo chorus plugin
├── tests/                    Unit tests for shared DSP
├── external/
│   └── JUCE/                 JUCE SDK (not in source control)
├── CMakeLists.txt
├── progress-dsp.md           DSP framework status and roadmap
└── README.md
```

---

## Architecture & Guiding Principles

- **DSP is independent from GUI** — no JUCE GUI headers in `shared/DSP/`
- **Realtime-safe** — no allocations, locks, or file I/O in `process()` / `processBlock()`
- **Composition over inheritance** — small, single-responsibility DSP classes in `shared/DSP/`
- **Consistent API** — every class exposes `prepare(sampleRate)`, `reset()`, and a `processSample()` or `process()` method
- **Namespace** — all shared DSP lives in `namespace bodsp`
- **Stable parameter IDs** — once shipped, parameter IDs do not change (APVTS-backed presets)
- **C++20** — `std::numbers::pi_v`, `std::clamp`, concepts-ready

---

## Roadmap

| Plugin / Module | Status |
|-----------------|--------|
| Distortion | ✅ Done |
| Delay | ✅ Done |
| Reverb | ✅ Done |
| Filter | ✅ Done |
| Chorus | ✅ Done |
| Compressor | 🔜 Planned (`EnvelopeFollower` ready) |
| Phaser | 🔜 Planned (`LFO` + all-pass chain) |
| Granular | 🔜 Planned |
| Oversampler FIR upgrade | 🔜 Planned |
| Preset system | 🔜 Planned |

---

## References

- [JUCE](https://juce.com/) — audio plugin framework
- [Freeverb](https://ccrma.stanford.edu/~jos/pasp/Freeverb.html) — reverb algorithm
- [Paul Kellett pink noise](http://www.firstpr.com.au/dsp/pink-noise/) — pink noise approximation
- [RBJ Audio Cookbook](https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html) — biquad filter design
