#pragma once

// Multi-mode biquad filter (Lowpass, Highpass, Low Shelf, High Shelf, Notch).
// Header-only, JUCE-free, real-time safe. Coefficients use the Audio EQ Cookbook
// formulas. State uses Direct Form II Transposed for numerical stability.
// Placed in namespace bodsp for reuse across plugins.

#include <cmath>
#include <vector>
#include <algorithm>
#include <numbers>

namespace bodsp
{

class BiquadFilter
{
public:
	enum class Mode
	{
		Lowpass,
		Highpass,
		LowShelf,
		HighShelf,
		Notch
	};

	BiquadFilter() noexcept = default;

	void prepare (double sampleRate, int numChannels)
	{
		sr = sampleRate;
		channels = std::max (1, numChannels);
		w1.assign ((size_t) channels, 0.0f);
		w2.assign ((size_t) channels, 0.0f);
		updateCoefficients();
	}

	void reset() noexcept
	{
		std::fill (w1.begin(), w1.end(), 0.0f);
		std::fill (w2.begin(), w2.end(), 0.0f);
	}

	void setMode (Mode m) noexcept
	{
		mode = m;
		updateCoefficients();
	}

	// Cutoff / centre frequency in Hz
	void setFrequency (float hz) noexcept
	{
		freq = std::clamp (hz, 10.0f, 20000.0f);
		updateCoefficients();
	}

	// Q / resonance (bandwidth for notch). Reasonable range: 0.1–10
	void setQ (float q) noexcept
	{
		Q = std::max (0.01f, q);
		updateCoefficients();
	}

	// Shelf gain in dB (only used by LowShelf / HighShelf)
	void setGainDb (float db) noexcept
	{
		gainDb = std::clamp (db, -36.0f, 36.0f);
		updateCoefficients();
	}

	// Process one sample per channel. Direct Form II Transposed.
	float processSample (int channel, float x) noexcept
	{
		if (channels <= 0) return x;
		const size_t ch = static_cast<size_t> (std::clamp (channel, 0, channels - 1));

		const float y = b0 * x + w1[ch];
		w1[ch] = b1 * x - a1 * y + w2[ch];
		w2[ch] = b2 * x - a2 * y;
		return y;
	}

	Mode    getMode()      const noexcept { return mode; }
	float   getFrequency() const noexcept { return freq; }
	float   getQ()         const noexcept { return Q; }
	float   getGainDb()    const noexcept { return gainDb; }

private:
	void updateCoefficients() noexcept
	{
		if (sr <= 0.0) return;

		const float w0    = 2.0f * std::numbers::pi_v<float> * freq / static_cast<float> (sr);
		const float cosW0 = std::cos (w0);
		const float sinW0 = std::sin (w0);
		const float alpha  = sinW0 / (2.0f * Q);
		const float A      = std::pow (10.0f, gainDb / 40.0f); // linear amplitude for shelves

		float b0r, b1r, b2r, a0r, a1r, a2r;

		switch (mode)
		{
			case Mode::Lowpass:
				b0r = (1.0f - cosW0) * 0.5f;
				b1r =  1.0f - cosW0;
				b2r = (1.0f - cosW0) * 0.5f;
				a0r =  1.0f + alpha;
				a1r = -2.0f * cosW0;
				a2r =  1.0f - alpha;
				break;

			case Mode::Highpass:
				b0r =  (1.0f + cosW0) * 0.5f;
				b1r = -(1.0f + cosW0);
				b2r =  (1.0f + cosW0) * 0.5f;
				a0r =   1.0f + alpha;
				a1r =  -2.0f * cosW0;
				a2r =   1.0f - alpha;
				break;

			case Mode::LowShelf:
			{
				const float sqrtA  = std::sqrt (A);
				const float alphaS = sinW0 * 0.5f * std::sqrt ((A + 1.0f / A) * (1.0f / Q - 1.0f) + 2.0f);
				b0r =       A * ((A + 1.0f) - (A - 1.0f) * cosW0 + 2.0f * sqrtA * alphaS);
				b1r = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosW0);
				b2r =       A * ((A + 1.0f) - (A - 1.0f) * cosW0 - 2.0f * sqrtA * alphaS);
				a0r =             (A + 1.0f) + (A - 1.0f) * cosW0 + 2.0f * sqrtA * alphaS;
				a1r = -2.0f *    ((A - 1.0f) + (A + 1.0f) * cosW0);
				a2r =             (A + 1.0f) + (A - 1.0f) * cosW0 - 2.0f * sqrtA * alphaS;
				break;
			}

			case Mode::HighShelf:
			{
				const float sqrtA  = std::sqrt (A);
				const float alphaS = sinW0 * 0.5f * std::sqrt ((A + 1.0f / A) * (1.0f / Q - 1.0f) + 2.0f);
				b0r =        A * ((A + 1.0f) + (A - 1.0f) * cosW0 + 2.0f * sqrtA * alphaS);
				b1r = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosW0);
				b2r =        A * ((A + 1.0f) + (A - 1.0f) * cosW0 - 2.0f * sqrtA * alphaS);
				a0r =              (A + 1.0f) - (A - 1.0f) * cosW0 + 2.0f * sqrtA * alphaS;
				a1r =  2.0f *     ((A - 1.0f) - (A + 1.0f) * cosW0);
				a2r =              (A + 1.0f) - (A - 1.0f) * cosW0 - 2.0f * sqrtA * alphaS;
				break;
			}

			case Mode::Notch:
			default:
				b0r =  1.0f;
				b1r = -2.0f * cosW0;
				b2r =  1.0f;
				a0r =  1.0f + alpha;
				a1r = -2.0f * cosW0;
				a2r =  1.0f - alpha;
				break;
		}

		// Normalize by a0
		b0 = b0r / a0r;
		b1 = b1r / a0r;
		b2 = b2r / a0r;
		a1 = a1r / a0r;
		a2 = a2r / a0r;
	}

	double sr { 44100.0 };
	int channels { 0 };

	Mode  mode    { Mode::Lowpass };
	float freq    { 1000.0f };
	float Q       { 0.707f };  // Butterworth default
	float gainDb  { 0.0f };

	// Normalized biquad coefficients
	float b0 { 1.0f }, b1 { 0.0f }, b2 { 0.0f };
	float a1 { 0.0f }, a2 { 0.0f };

	// DF2T state per channel
	std::vector<float> w1, w2;
};

} // namespace bodsp
