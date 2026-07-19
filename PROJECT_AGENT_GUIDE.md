# Project Agent Guide — BoSP

Purpose
-------
This guide explains how an automated agent (or human contributor) should work on BoSP while maintaining code quality and CI safety.

Agent responsibilities
 - Operate on a feature branch for every change.
 - Make small, testable commits with clear messages (scope: 1–3 files).
 - Run the project's build and unit tests before creating a PR.
 - Respect coding standards and the shared/DSP boundaries.

Safe workflows
-------------
1. Open workspace and run a full CMake configure from the repository root (recommended):
   cmake -S . -B build

   If you prefer Ninja explicitly, pass -G "Ninja".
2. Build the Debug configuration (from repo root):
   cmake --build build --config Debug
   cmake --build build --target test
3. Make incremental changes. Keep processBlock() free of allocations.

Testing & verification
----------------------
- Unit tests should cover shared/DSP algorithms (deterministic).
- Audio QA: use the standalone build or host to verify parameter behavior and absence of clicks on parameter changes.

Committing & PRs
----------------
- Commit messages: <area>: short description — body (if necessary)
- Create PR against master; include testing steps and audible behavior notes.

Safety rules for agents
-----------------------
- Never modify external/JUCE in-place.
- When adding C++ source, update CMake only if necessary and keep changes minimal.
- Do not auto-approve or auto-merge; human review required for audio behavior/regressions.

Using a different agent (Gemini)
--------------------------------
- If you switch to a different automation agent (for example, Gemini), follow the same safety rules.
- Ensure the agent runs the same local steps before proposing changes: cmake configure, build Debug/Release, and run unit tests.
- Provide the agent with a short-lived feature branch pattern: feature/<agent>/<short-desc> so changes are easy to review and revert.
- Do not store credentials or large binary artefacts in the repository; the agent should use environment-provided secrets if needed.

Agent checklist (for any agent)
------------------------------
1. Run `cmake -S . -B build` and ensure configuration passes.
2. Build Debug and Release, and run unit tests: `cmake --build build --config Debug` then `cmake --build build --config Release`.
3. Run audio smoke tests (manual or automated host tests) for parameter propagation on Delay and Distortion.
4. Create a PR with a clear summary, list of changed files, and testing steps. Attach CI logs if available.

If you want, include a small manifest in the PR body listing the agent's runtime and model (e.g., "agent: Gemini, model: Gemini Pro"), so reviewers know which agent produced the changes.
