#pragma once

// Simple, allocation-free gain DSP used by processors.
// This class lives in namespace bodsp and has no dependencies on JUCE GUI.

#include <cmath>

namespace bodsp
{
class Gain
{
public:
	Gain() noexcept = default;

	// Prepare the processor. Present for API consistency; doesn't allocate.
	void prepare (double /*sampleRate*/) noexcept {}

	// Reset internal state (no-op for simple gain)
	void reset() noexcept {}

	// Set gain by linear multiplier (no allocation)
	void setGainLinear (float newGain) noexcept { gain = newGain; }

	// Set gain in decibels
	void setGainDecibels (float db) noexcept { gain = std::pow (10.0f, db * 0.05f); }

	// Process a single channel in-place. Non-allocating.
	void process (float* samples, int numSamples) noexcept
	{
		if (gain == 1.0f)
			return;

		for (int i = 0; i < numSamples; ++i)
			samples[i] *= gain;
	}

	// Process interleaved channels is intentionally not provided; processors should
	// call process per-channel to avoid dependencies on buffer layouts.

private:
	float gain = 1.0f;
};

} // namespace bodsp
