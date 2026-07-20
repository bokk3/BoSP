#pragma once

/// bodsp::DCBlocker — One-pole DC removal filter, one instance per channel.
/// Formula: y[n] = x[n] - x[n-1] + R * y[n-1]
/// where R = 1 - (2 * pi * cutoffHz / sampleRate).
/// Realtime safe: no allocations in processSample(). JUCE-free.

#include <cmath>
#include <numbers>
#include <vector>
#include <algorithm>

namespace bodsp
{

class DCBlocker
{
public:
	DCBlocker() noexcept = default;

	/// Prepare per-channel state. Allocates state vectors once.
	void prepare (double sampleRate, int numChannels)
	{
		sr       = sampleRate;
		channels = std::max (1, numChannels);
		xPrev.assign (static_cast<size_t> (channels), 0.0f);
		yPrev.assign (static_cast<size_t> (channels), 0.0f);
		updateCoefficient();
	}

	/// Cutoff frequency in Hz (default 5 Hz). Call before prepare() or after.
	void setCutoffHz (float hz) noexcept
	{
		cutoffHz = std::clamp (hz, 1.0f, 200.0f);
		updateCoefficient();
	}

	void reset() noexcept
	{
		std::fill (xPrev.begin(), xPrev.end(), 0.0f);
		std::fill (yPrev.begin(), yPrev.end(), 0.0f);
	}

	/// Process one sample for the given channel. Realtime safe, no allocations.
	[[nodiscard]] float processSample (int channel, float x) noexcept
	{
		const size_t ch = static_cast<size_t> (std::clamp (channel, 0, channels - 1));
		const float y = x - xPrev[ch] + R * yPrev[ch];
		xPrev[ch] = x;
		yPrev[ch] = y;
		return y;
	}

	float getCutoffHz() const noexcept { return cutoffHz; }

private:
	void updateCoefficient() noexcept
	{
		if (sr > 0.0)
			R = 1.0f - (2.0f * std::numbers::pi_v<float> * cutoffHz / static_cast<float> (sr));
		R = std::clamp (R, 0.0f, 0.9999f);
	}

	double sr        { 44100.0 };
	int    channels  { 0 };
	float  cutoffHz  { 5.0f };
	float  R         { 0.999f };

	// Per-channel state — allocated once in prepare()
	std::vector<float> xPrev;
	std::vector<float> yPrev;
};

} // namespace bodsp
