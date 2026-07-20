#pragma once

/// bodsp::Oversampler — Simple 2x/4x oversampling for use in nonlinear processing.
///
/// Architecture:
///   upsample()  — expand input block into internal oversampled buffer
///   [caller processes getOversampledBuffer() at oversampled rate]
///   downsample() — decimate back to base rate output buffer
///
/// Current implementation uses linear interpolation for upsampling and
/// averaging for downsampling. This is a clean baseline.
///
/// Extension point: replace linearUpsample / averageDownsample with
/// polyphase FIR half-band filters for production-quality oversampling.
///
/// Realtime safe: internal buffer allocated ONCE in prepare(). JUCE-free.

#include <vector>
#include <algorithm>
#include <cmath>

namespace bodsp
{

enum class OversamplingFactor { x1 = 1, x2 = 2, x4 = 4 };

class Oversampler
{
public:
	Oversampler() noexcept = default;

	/// Prepare internal buffers. Call before processing.
	/// maxBlockSize is the maximum base-rate block size expected.
	void prepare (double baseSampleRate, int maxBlockSize, OversamplingFactor f)
	{
		factor       = static_cast<int> (f);
		baseSR       = baseSampleRate;
		maxBlock     = maxBlockSize;

		const int oversampledSize = maxBlockSize * factor;
		internalBuf.assign (static_cast<size_t> (oversampledSize), 0.0f);
		reset();
	}

	void reset() noexcept
	{
		std::fill (internalBuf.begin(), internalBuf.end(), 0.0f);
		lastSample = 0.0f;
	}

	/// Upsample input block into internal oversampled buffer.
	/// 'numSamples' is the base-rate sample count.
	void upsample (const float* input, int numSamples) noexcept
	{
		if (factor == 1)
		{
			for (int i = 0; i < numSamples; ++i)
				internalBuf[static_cast<size_t> (i)] = input[i];
			return;
		}

		// --- Linear interpolation upsampling ---
		// Extension point: replace with polyphase FIR half-band filter
		for (int i = 0; i < numSamples; ++i)
		{
			const float prev = (i == 0) ? lastSample : input[i - 1];
			const float curr = input[i];
			const int base = i * factor;
			for (int k = 0; k < factor; ++k)
			{
				const float t = static_cast<float> (k) / static_cast<float> (factor);
				internalBuf[static_cast<size_t> (base + k)] = prev + t * (curr - prev);
			}
		}
		lastSample = input[numSamples - 1];
	}

	/// Returns pointer to the oversampled buffer for in-place processing.
	float* getOversampledBuffer() noexcept { return internalBuf.data(); }
	const float* getOversampledBuffer() const noexcept { return internalBuf.data(); }

	/// Number of samples in the oversampled buffer (numSamples * factor).
	int getOversampledSize (int baseNumSamples) const noexcept { return baseNumSamples * factor; }

	/// Downsample internal buffer back into output (base-rate).
	void downsample (float* output, int numSamples) noexcept
	{
		if (factor == 1)
		{
			for (int i = 0; i < numSamples; ++i)
				output[i] = internalBuf[static_cast<size_t> (i)];
			return;
		}

		// --- Averaging decimation ---
		// Extension point: replace with polyphase FIR half-band filter
		const float invFactor = 1.0f / static_cast<float> (factor);
		for (int i = 0; i < numSamples; ++i)
		{
			float sum = 0.0f;
			const int base = i * factor;
			for (int k = 0; k < factor; ++k)
				sum += internalBuf[static_cast<size_t> (base + k)];
			output[i] = sum * invFactor;
		}
	}

	double getOversampledSampleRate() const noexcept
	{
		return baseSR * static_cast<double> (factor);
	}

	int getOversamplingFactor() const noexcept { return factor; }

private:
	int    factor    { 1 };
	double baseSR    { 44100.0 };
	int    maxBlock  { 0 };
	float  lastSample{ 0.0f };

	// Internal oversampled buffer — allocated once in prepare()
	std::vector<float> internalBuf;
};

} // namespace bodsp
