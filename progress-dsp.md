# BoDSP Shared DSP Framework — Progress

> All modules live in `shared/DSP/`. Namespace `bodsp`.

---

## Pre-Existing Shared Libraries

> Built before the new framework. These are the foundation plugins are built on.

| File | Description | Used By |
|------|-------------|---------|
| `Gain.h` | Linear gain stage, per-channel | Distortion, Delay |
| `WaveShaper.h` | Soft / Tube / Hard / Fold waveshaping algorithms | Distortion |
| `Tone.h` | One-pole LP tone control | Distortion |
| `Delay.h` | Full delay with LP/HP filters, auto-duck (envelope follower), feedback | Delay |
| `DelayLine.h` | Low-level circular buffer delay line | Delay.h (internal) |
| `Reverb.h` | Freeverb-based stereo reverb with pre-delay, LP/HP filters | Reverb |
| `BiquadFilter.h` | Biquad filter — LP, HP, Low Shelf, High Shelf, Notch | Filter |
| `OnePoleFilters.h` | One-pole LP/HP, used inside Delay and Reverb | Delay.h, Reverb.h |

---

## New Shared DSP Framework

> C++20, JUCE-free, header-only, realtime-safe.

| # | Module | File | Status |
|---|--------|------|--------|
| 1 | Dry/Wet Mixer | `DryWet.h` | ✅ Done |
| 2 | Parameter Smoother | `ParameterSmoother.h` | ✅ Done |
| 3 | Envelope Follower | `EnvelopeFollower.h` | ✅ Done |
| 4 | LFO | `LFO.h` | ✅ Done |
| 5 | DC Blocker | `DCBlocker.h` | ✅ Done |
| 6 | Soft Clipper | `SoftClipper.h` | ✅ Done |
| 7 | Noise Generator | `NoiseGenerator.h` | ✅ Done |
| 8 | Meter | `Meter.h` | ✅ Done |
| 9 | Stereo Tools | `StereoTools.h` | ✅ Done |
| 10 | Oversampler | `Oversampler.h` | ✅ Done |

---

## Design Principles

- **C++20** throughout (`std::numbers`, concepts-ready)
- **JUCE-independent** — no JUCE headers in new modules
- **Header-only** — no .cpp required
- **No heap allocations** in `process()` / hot paths (allocated once in `prepare()`)
- **Consistent API** — every class has `prepare()`, `reset()`, and a `process*()` method
- **Namespace** — `bodsp`
- **No GUI code** — `Meter` exposes raw values via atomics only

---

## API Cheatsheet

```cpp
// DryWet
DryWet dw;
dw.setMode(DryWet::CrossfadeMode::EqualPower);
dw.setMix(0.5f);
float out = dw.process(dry, wet);

// ParameterSmoother
ParameterSmoother ps;
ps.prepare(sampleRate);
ps.setMode(ParameterSmoother::SmootherMode::Exponential);
ps.setTimeConstant(50.0f); // 50ms
ps.setTarget(newValue);
float v = ps.getNextValue(); // call once per sample

// EnvelopeFollower
EnvelopeFollower ef;
ef.prepare(sampleRate);
ef.setMode(EnvelopeFollower::FollowerMode::Peak);
ef.setAttackMs(5.0f);
ef.setReleaseMs(100.0f);
float env = ef.processSample(x);

// LFO
LFO lfo;
lfo.prepare(sampleRate);
lfo.setWaveform(LFO::LFOWaveform::Sine);
lfo.setRateHz(2.0f);
lfo.setDepth(1.0f);
float mod = lfo.getNextValue();

// DCBlocker
DCBlocker dc;
dc.prepare(sampleRate, 2);
float y = dc.processSample(ch, x);

// SoftClipper
SoftClipper sc;
sc.setMode(SoftClipper::ClipMode::Tanh);
sc.setInputGainDb(6.0f);
float clipped = sc.processSample(x);

// NoiseGenerator
NoiseGenerator ng;
ng.setType(NoiseGenerator::NoiseType::Pink);
ng.setGain(0.01f);
float noise = ng.getNextSample();

// Meter
Meter m;
m.prepare(sampleRate);
m.processSample(x);           // call in process loop
float peakDb = m.getPeakDb(); // call from GUI timer

// StereoTools
StereoTools st;
st.setWidth(1.5f);
st.process(L, R);
// or stateless:
StereoTools::encodeMidSide(L, R, M, S);
StereoTools::decodeMidSide(M, S, L, R);

// Oversampler
Oversampler os;
os.prepare(sampleRate, blockSize, OversamplingFactor::x4);
os.upsample(input, numSamples);
float* buf = os.getOversampledBuffer();
// ... process buf at 4x rate ...
os.downsample(output, numSamples);
```

---

## Plugin DSP Module Usage

| Module | Distortion | Delay | Reverb | Filter | Chorus | Compressor |
|--------|:---:|:---:|:---:|:---:|:---:|:---:|
| Gain | ✅ | ✅ | — | — | — | ✅ |
| WaveShaper | ✅ | — | — | — | — | — |
| Tone | ✅ | — | — | — | — | — |
| Delay | — | ✅ | — | — | — | — |
| DelayLine | — | (internal) | — | — | ✅ | — |
| Reverb | — | — | ✅ | — | — | — |
| BiquadFilter | — | (internal) | (internal) | ✅ | — | — |
| OnePoleFilters | — | (internal) | (internal) | — | — | — |
| DryWet | ✅ | — | — | ✅ | ✅ | ✅ |
| ParameterSmoother | ✅ | — | — | — | ✅ | ✅ |
| EnvelopeFollower | — | (internal) | — | — | — | ✅ |
| LFO | — | — | — | — | ✅ | — |
| DCBlocker | — | — | — | — | — | — |
| SoftClipper | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| NoiseGenerator | — | — | — | — | — | — |
| Meter | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| StereoTools | — | — | — | — | — | — |
| Oversampler | — | — | — | — | — | — |

---

## Next Steps

- [ ] Write unit tests for each new DSP module in `tests/`
- [ ] Upgrade `Oversampler` with polyphase half-band FIR filters
- [x] Implement `Compressor` plugin (uses `EnvelopeFollower`, `ParameterSmoother`, `Meter`, `Gain`)
- [ ] Implement `Phaser` plugin (uses `LFO` + all-pass chain)
- [ ] Implement `Granular` plugin

