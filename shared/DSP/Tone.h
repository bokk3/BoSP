#pragma once

// Simple one-pole lowpass tone filter. Header-only, no GUI deps.

#include <vector>
#include <cmath>
#include <numbers>

namespace bodsp
{
class Tone
{
public:
	Tone() noexcept = default;

	void prepare (double sampleRate, int numChannels)
	{
		fs = static_cast<float> (sampleRate);
		channels = numChannels;
		z1.assign ((size_t) numChannels, 0.0f);
		updateCoefficients();
	}

	void reset() noexcept
	{
		std::fill (z1.begin(), z1.end(), 0.0f);
	}

	void setCutoffHz (float hz) noexcept
	{
		cutoff = hz;
		updateCoefficients();
	}

	// Process a single sample for a given channel
	float processSample (int channel, float x) noexcept
	{
		if (channel < 0 || channel >= channels)
			return x;

		// y[n] = (1 - a) * x + a * y[n-1]
		const float y = (1.0f - a) * x + a * z1[(size_t) channel];
		z1[(size_t) channel] = y;
		return y;
	}

private:
	void updateCoefficients() noexcept
	{
		if (fs <= 0.0f)
			return;

		const float wc = 2.0f * std::numbers::pi_v<float> * cutoff;
		// Simple RC filter smoothing factor
		const float dt = 1.0f / fs;
		const float rc = 1.0f / wc;
		a = rc / (rc + dt);
	}

	float fs { 44100.0f };
	int channels { 2 };
	float cutoff { 10000.0f };
	float a { 0.999f };
	std::vector<float> z1;
};

} // namespace bodsp
