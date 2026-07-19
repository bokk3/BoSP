#pragma once

// Simple one-pole lowpass and highpass filters, header-only and real-time safe.

#include <vector>
#include <cmath>
#include <algorithm>
#include <numbers>

namespace bodsp
{
class OnePoleLowpass
{
public:
	OnePoleLowpass() noexcept = default;

	void prepare (double sampleRate, int numChannels)
	{
		sr = sampleRate;
		channels = std::max (1, numChannels);
		z1.assign ((size_t) channels, 0.0f);
		cutoffHz = 20000.0f;
		updateCoef();
	}

	void reset() noexcept
	{
		std::fill (z1.begin(), z1.end(), 0.0f);
	}

	void setCutoffHz (float hz) noexcept
	{
		cutoffHz = hz;
		updateCoef();
	}

	float processSample (int channel, float x) noexcept
	{
		if (channels <= 0) return x;
		const size_t ch = static_cast<size_t> (std::clamp (channel, 0, channels - 1));
		// y[n] = y[n-1] + alpha * (x - y[n-1])
		const float y = z1[ch] + alpha * (x - z1[ch]);
		z1[ch] = y;
		return y;
	}

private:
	void updateCoef() noexcept
	{
		if (sr <= 0.0) return;
		const float dt = 1.0f / static_cast<float> (sr);
		const float rc = 1.0f / (2.0f * std::numbers::pi_v<float> * cutoffHz);
		alpha = dt / (rc + dt);
	}

	double sr { 44100.0 };
	int channels { 0 };
	float cutoffHz { 20000.0f };
	float alpha { 1.0f };
	std::vector<float> z1;
};

class OnePoleHighpass
{
public:
	OnePoleHighpass() noexcept = default;

	void prepare (double sampleRate, int numChannels)
	{
		sr = sampleRate;
		channels = std::max (1, numChannels);
		y1.assign ((size_t) channels, 0.0f);
		x1.assign ((size_t) channels, 0.0f);
		cutoffHz = 20.0f;
		updateCoef();
	}

	void reset() noexcept
	{
		std::fill (y1.begin(), y1.end(), 0.0f);
		std::fill (x1.begin(), x1.end(), 0.0f);
	}

	void setCutoffHz (float hz) noexcept
	{
		cutoffHz = hz;
		updateCoef();
	}

	float processSample (int channel, float x) noexcept
	{
		if (channels <= 0) return x;
		const size_t ch = static_cast<size_t> (std::clamp (channel, 0, channels - 1));
		// y[n] = a * (y[n-1] + x[n] - x[n-1])
		const float y = a * (y1[ch] + x - x1[ch]);
		y1[ch] = y;
		x1[ch] = x;
		return y;
	}

private:
	void updateCoef() noexcept
	{
		if (sr <= 0.0) return;
		const float dt = 1.0f / static_cast<float> (sr);
		const float rc = 1.0f / (2.0f * std::numbers::pi_v<float> * cutoffHz);
		// coefficient for the classical one-pole highpass discrete form
		a = rc / (rc + dt);
	}

	double sr { 44100.0 };
	int channels { 0 };
	float cutoffHz { 20.0f };
	float a { 0.0f };
	std::vector<float> y1; // output state
	std::vector<float> x1; // input previous sample
};

} // namespace bodsp
