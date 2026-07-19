# BoSP

BoSP is a collection of professional audio plugins and a shared DSP library built with JUCE and CMake.

Purpose
-------
This repository provides a clean, reusable architecture for building high-quality VST3 plugins. The codebase favors modern C++ (C++20), testability, and a strict separation between DSP and GUI.

Quick start (Windows / MSVC)
-----------------------------
1. Ensure prerequisites: Visual Studio (C++), CMake 3.22+, JUCE in external/JUCE
2. Configure (from repository root):

   cmake -S . -B build

   This will generate the build system for your default generator (Visual Studio on Windows). If you prefer Ninja you can pass -G "Ninja".

3. Build the Debug configuration (from repository root):

   cmake --build build --config Debug

Notes
- To build a different configuration, replace Debug with Release.
- To build a specific plugin target (e.g., only the Distortion plugin) you can append --target <target-name>. Example:

  cmake --build build --config Debug --target BoDSPDistortion_VST3

 - The built .vst3 artifact is typically placed under build/plugins/Distortion/BoDSPDistortion_artefacts/<Config>/

Project layout
--------------
- shared/DSP          Reusable DSP components (Gain, WaveShaper, etc.)
- plugins/Distortion   Distortion plugin (processor + editor)
- external/JUCE        JUCE SDK (kept out of source control)
- tests/               Unit tests for shared DSP

Guiding principles
-------------------
- DSP is independent from GUI and contains no JUCE GUI headers.
- No allocations, locks, or file I/O inside the audio thread.
- Prefer composition and single-responsibility components in shared/DSP.
- Parameter IDs are stable once released. Use juce::AudioProcessorValueTreeState.

Where to go next
-----------------
- See PROJECT_AGENT_GUIDE.md for automation/agent instructions.
- See DEVELOPMENT_PLAN.md and ROADMAP.md for milestones and priorities.

