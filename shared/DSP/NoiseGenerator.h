#pragma once

/// bodsp::NoiseGenerator — White and pink noise generator.
/// Pink noise uses Paul Kellett's 3-variable approximation.
/// Realtime safe: no allocations in getNextSample(). JUCE-free.

#include <random>
#include <cmath>
#include <algorithm>

namespace bodsp
{

class NoiseGenerator
{
public:
	enum class NoiseType { White, Pink };

	NoiseGenerator() noexcept
	{
		rng.seed (0xB0D5FEED);
	}

	void setType (NoiseType t) noexcept { type = t; }

	/// Linear gain multiplier applied to output.
	void setGain (float linearGain) noexcept { gain = linearGain; }

	/// No internal buffers, but kept for API consistency.
	void prepare (double /*sampleRate*/) noexcept {}

	void reset() noexcept
	{
		b0 = b1 = b2 = 0.0f;
	}

	/// Return the next noise sample scaled by gain. Realtime safe.
	[[nodiscard]] float getNextSample() noexcept
	{
		// Uniform white noise in [-1, 1]
		std::uniform_real_distribution<float> dist (-1.0f, 1.0f);
		const float white = dist (rng);

		if (type == NoiseType::White)
			return white * gain;

		// Pink noise — Paul Kellett 3-filter approximation
		// Coefficients tuned for natural pink spectrum
		b0 = 0.99886f * b0 + white * 0.0555179f;
		b1 = 0.99332f * b1 + white * 0.0750759f;
		b2 = 0.96900f * b2 + white * 0.1538520f;

		// Sum + residual white, then scale to approximate unity gain
		const float pink = (b0 + b1 + b2 + white * 0.5362f) * 0.11f;
		return std::clamp (pink, -1.0f, 1.0f) * gain;
	}

private:
	NoiseType       type  { NoiseType::White };
	float           gain  { 1.0f };

	// Pink filter state
	float           b0    { 0.0f };
	float           b1    { 0.0f };
	float           b2    { 0.0f };

	std::minstd_rand rng;
};

} // namespace bodsp
