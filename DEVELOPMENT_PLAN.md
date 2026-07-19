# DEVELOPMENT PLAN — BoSP Distortion

Objective
---------
Deliver a small suite of professional audio plugins (Distortion, Delay) with a reusable shared DSP library and test coverage. Keep the shared/DSP layer free of GUI dependencies and suitable for reuse across plugins.

Milestones
----------
v0.1 — Pass-through (Distortion) (DONE)
v0.2 — Distortion: Drive parameter + smoothing (DONE)
v0.3 — Distortion: WaveShaper modes (Soft/Tube/Hard/Fold) (IN PROGRESS)
v0.4 — Distortion: Output gain, Dry/Wet, Editor attachments (IN PROGRESS)
v0.5 — Distortion: Tone filter (post-waveshaper)
v0.6 — Delay: Basic multifunctional delay (mono/stereo/ping-pong) with auto-duck, LP/HP feedback filtering (DONE)
v0.7 — Delay: Fixes and QA; ensure parameter propagation and no clicks (DONE)
v0.8 — Shared/DSP: Add unit tests for DelayLine, OnePole filters, and WaveShaper
v1.0 — Release candidate: polish, comprehensive tests, documentation

Sprint tasks (next 2 weeks)
---------------------------
1. Finish WaveShaper mode edge cases and add unit tests for WaveShaper (2 days)
2. Add DelayLine unit tests: interpolation and wrap tests (1 day)
3. Add CI job that builds both plugins and runs unit tests (1 day)
4. Documentation: update README, add a brief DSP architecture doc (1 day)

Acceptance criteria
-------------------
- CI builds Debug and Release without warnings.
- Unit tests covering core DSP (DelayLine, filters, WaveShaper) pass.
- No clicks or artifacts when automating time/feedback/mix in Delay and Drive/Output in Distortion.
- Editor controls layout and behavior verified manually for common hosts.

Risks
-----
- Aliasing from waveshaping: mitigated by planned oversampling and post-filtering.
- Parameter propagation bugs: mitigate by sampling APVTS in each processBlock and unit testing.
