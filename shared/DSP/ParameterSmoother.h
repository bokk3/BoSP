#pragma once

/// bodsp::ParameterSmoother — Zipper-noise-free parameter smoothing.
/// Supports linear ramp and exponential (one-pole) modes.
/// Realtime safe: no allocations in getNextValue(). JUCE-free.

#include <cmath>
#include <algorithm>

namespace bodsp
{

class ParameterSmoother
{
public:
	enum class SmootherMode { Linear, Exponential };

	ParameterSmoother() noexcept = default;

	void prepare (double sampleRate) noexcept
	{
		sr = sampleRate;
		reset (currentValue);
	}

	/// Choose smoothing algorithm.
	void setMode (SmootherMode m) noexcept { mode = m; }

	/// Linear: time for a full 0→1 ramp in seconds.
	void setRampTime (float seconds) noexcept
	{
		rampTimeSamples = static_cast<int> (seconds * static_cast<float> (sr));
		if (rampTimeSamples < 1) rampTimeSamples = 1;
	}

	/// Exponential: time constant in milliseconds.
	/// coeff = exp(-1 / (ms * 0.001 * sampleRate))
	void setTimeConstant (float ms) noexcept
	{
		const float tau = std::max (0.01f, ms) * 0.001f * static_cast<float> (sr);
		expCoeff = std::exp (-1.0f / tau);
	}

	/// Set the value to smooth toward.
	void setTarget (float t) noexcept
	{
		target = t;
		if (mode == SmootherMode::Linear)
		{
			delta       = (target - currentValue) / static_cast<float> (rampTimeSamples);
			stepsLeft   = rampTimeSamples;
		}
	}

	/// Snap immediately to value, no smoothing.
	void reset (float value = 0.0f) noexcept
	{
		currentValue = value;
		target       = value;
		stepsLeft    = 0;
		delta        = 0.0f;
	}

	/// Advance one sample and return the smoothed value. Realtime safe.
	[[nodiscard]] float getNextValue() noexcept
	{
		if (mode == SmootherMode::Linear)
		{
			if (stepsLeft > 0)
			{
				currentValue += delta;
				--stepsLeft;
				// Clamp on last step to avoid float drift
				if (stepsLeft == 0)
					currentValue = target;
			}
		}
		else
		{
			// Exponential: first-order IIR
			currentValue = expCoeff * currentValue + (1.0f - expCoeff) * target;
		}
		return currentValue;
	}

	bool  isSmoothing()    const noexcept { return (mode == SmootherMode::Linear) ? (stepsLeft > 0) : (std::abs (currentValue - target) > 1e-6f); }
	float getCurrentValue() const noexcept { return currentValue; }
	float getTarget()       const noexcept { return target; }

private:
	SmootherMode mode            { SmootherMode::Exponential };
	double       sr              { 44100.0 };
	float        currentValue    { 0.0f };
	float        target          { 0.0f };

	// Linear state
	int   rampTimeSamples { 4410 };
	int   stepsLeft       { 0 };
	float delta           { 0.0f };

	// Exponential state
	float expCoeff        { 0.9f };
};

} // namespace bodsp
