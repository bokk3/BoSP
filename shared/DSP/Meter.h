#pragma once

/// bodsp::Meter — Non-GUI, realtime audio analysis meter.
/// Tracks instantaneous Peak, windowed RMS, Peak Hold with timed decay,
/// and clip detection. All getters return plain floats — render in the GUI
/// as you see fit.
/// Realtime safe: RMS window buffer allocated ONCE in prepare(). JUCE-free.

#include <cmath>
#include <vector>
#include <algorithm>
#include <atomic>

namespace bodsp
{

class Meter
{
public:
	Meter() noexcept = default;

	/// Prepare internal state. Allocates RMS window buffer (once).
	void prepare (double sampleRate)
	{
		sr = sampleRate;

		// RMS window: 300 ms
		const int windowSamples = static_cast<int> (sampleRate * 0.30);
		rmsBuffer.assign (static_cast<size_t> (windowSamples), 0.0f);
		rmsWritePos = 0;
		rmsSum      = 0.0;

		holdTimeSamples = static_cast<int> (holdTimeMs * 0.001 * sampleRate);
		holdCounter     = 0;

		reset();
	}

	/// Peak hold time in milliseconds (default 1000 ms).
	void setHoldTimeMs (float ms) noexcept
	{
		holdTimeMs      = std::max (0.0f, ms);
		holdTimeSamples = static_cast<int> (holdTimeMs * 0.001f * static_cast<float> (sr));
	}

	void reset() noexcept
	{
		std::fill (rmsBuffer.begin(), rmsBuffer.end(), 0.0f);
		rmsSum      = 0.0;
		rmsWritePos = 0;

		peak.store (0.0f);
		rmsOut.store (0.0f);
		peakHold.store (0.0f);
		clipped.store (false);
		holdCounter = 0;
	}

	/// Feed one sample into the meter. Call once per sample from the audio thread.
	void processSample (float x) noexcept
	{
		const float abs_x = std::abs (x);

		// Instantaneous peak
		peak.store (abs_x);

		// Clip detection
		if (abs_x > 1.0f + 1e-4f)
			clipped.store (true);

		// Peak hold
		if (abs_x >= peakHold.load())
		{
			peakHold.store (abs_x);
			holdCounter = holdTimeSamples;
		}
		else if (holdCounter > 0)
		{
			--holdCounter;
		}
		else
		{
			// Slow exponential decay after hold time expires
			peakHold.store (peakHold.load() * 0.9995f);
		}

		// Windowed RMS: maintain running sum-of-squares in circular buffer
		if (!rmsBuffer.empty())
		{
			const size_t pos   = static_cast<size_t> (rmsWritePos);
			const float old    = rmsBuffer[pos];
			const float sq     = x * x;
			rmsSum            += static_cast<double> (sq - old);
			rmsBuffer[pos]     = sq;
			rmsWritePos        = static_cast<int> ((pos + 1) % rmsBuffer.size());

			const double meanSq = rmsSum / static_cast<double> (rmsBuffer.size());
			rmsOut.store (std::sqrt (static_cast<float> (std::max (0.0, meanSq))));
		}
	}

	// --- Getters (safe to call from GUI thread) ---

	float getPeak()      const noexcept { return peak.load(); }
	float getRMS()       const noexcept { return rmsOut.load(); }
	float getPeakHold()  const noexcept { return peakHold.load(); }
	bool  hasClipped()   const noexcept { return clipped.load(); }
	void  clearClipped() noexcept       { clipped.store (false); }

	float getPeakDb()     const noexcept { return toDb (peak.load()); }
	float getRMSDb()      const noexcept { return toDb (rmsOut.load()); }
	float getPeakHoldDb() const noexcept { return toDb (peakHold.load()); }

private:
	static float toDb (float linear) noexcept
	{
		return (linear > 1e-9f) ? 20.0f * std::log10 (linear) : -120.0f;
	}

	double sr            { 44100.0 };
	float  holdTimeMs    { 1000.0f };
	int    holdTimeSamples{ 44100 };
	int    holdCounter   { 0 };

	// RMS circular buffer (allocated once in prepare)
	std::vector<float> rmsBuffer;
	double             rmsSum      { 0.0 };
	int                rmsWritePos { 0 };

	// Atomic outputs: audio thread writes, GUI thread reads
	std::atomic<float> peak     { 0.0f };
	std::atomic<float> rmsOut   { 0.0f };
	std::atomic<float> peakHold { 0.0f };
	std::atomic<bool>  clipped  { false };
};

} // namespace bodsp
