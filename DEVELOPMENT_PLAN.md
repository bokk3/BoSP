# DEVELOPMENT PLAN — BoSP Distortion

Objective
---------
Deliver a professional, production-ready Distortion plugin (VST3) with a reusable shared DSP library.

Milestones
----------
v0.1 — Pass-through (DONE)
v0.2 — Drive parameter + smoothing (DONE)
v0.3 — WaveShaper with modes (Soft/Tube/Hard/Fold) (IN PROGRESS)
v0.4 — Output gain, Dry/Wet, Editor attachments (IN PROGRESS)
v0.5 — Tone filter (post-waveshaper)
v0.6 — Oversampling and anti-alias filtering
v0.7 — UI polish and accessible presets
v1.0 — Release candidate: polish, comprehensive tests, documentation

Sprint tasks (next 2 weeks)
---------------------------
1. Wire 'mode' parameter into processor and ensure thread-safe updates. (1 day)
2. Add Dry/Wet parameter + small shared/DSP DryWet class. (2 days)
3. Add Tone: simple 2-pole filter in shared/DSP and connect as post-waveshaper. (3 days)
4. Integrate unit tests into top-level CMake & CI. (1 day)

Acceptance criteria
-------------------
- No ASSERTs or warnings in Release build on CI.
- Unit tests for WaveShaper pass.
- No clicks when automating Drive or Output (smoothed).
- Editor controls do not overlap at common sizes; layout responsive.

Risks
-----
- Aliasing from waveshaping: mitigated by planned oversampling and post-filtering.
- Parameter ID changes: prevent by locking parameter IDs once released.
