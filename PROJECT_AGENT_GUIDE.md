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
