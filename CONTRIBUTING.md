# Contributing to BoSP

Thank you for contributing. Follow these rules to keep the project maintainable:

Code style
- C++20
- Opening braces on the same line.
- Use const where possible.
- No raw new/delete; prefer std::unique_ptr / stack objects.

Audio thread rules
- No allocations, locks, or file I/O in processBlock().
- Avoid calling slow math inside per-sample loops where possible.

Parameters
- Use juce::AudioProcessorValueTreeState for automatable parameters.
- Parameter IDs are permanent once released.

PR checklist
- Build succeeds (Debug/Release) on Windows.
- New public APIs documented.
- Unit tests added for algorithmic logic.
- PR description includes testing steps and expected audible behavior.
