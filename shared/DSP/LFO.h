#pragma once

/// bodsp::LFO — Low-frequency oscillator: Sine, Triangle, Saw, Square, Sample & Hold.
/// Sync-ready architecture (external phase reset support).
/// Realtime safe: no allocations in getNextValue(). JUCE-free.

#include <cmath>
#include <numbers>
#include <random>
#include <algorithm>

namespace bodsp
{

class LFO
{
public:
	enum class LFOWaveform { Sine, Triangle, Saw, Square, SampleAndHold };

	LFO() noexcept
	{
		rng.seed (12345);
	}

	void prepare (double sampleRate) noexcept
	{
		sr          = sampleRate;
		updatePhaseInc();
	}

	void setWaveform (LFOWaveform w) noexcept { waveform = w; }

	void setRateHz (float hz) noexcept
	{
		rate = std::max (0.001f, hz);
		updatePhaseInc();
	}

	/// Depth: output scales by this amount. 0..1 recommended but not enforced.
	void setDepth (float d) noexcept { depth = d; }

	/// Phase offset in [0, 1] (maps to 0..2pi).
	void setPhaseOffset (float offset) noexcept
	{
		phaseOffset = offset - std::floor (offset); // wrap to 0..1
	}

	/// If true: output range is -depth..+depth. If false: 0..depth.
	void setBipolar (bool b) noexcept { bipolar = b; }

	void reset() noexcept
	{
		phase    = 0.0f;
		sHoldVal = 0.0f;
	}

	/// Sync: reset phase to 0 (e.g. on host transport start).
	void sync() noexcept { reset(); }

	/// Advance one sample, return the next LFO value.
	[[nodiscard]] float getNextValue() noexcept
	{
		const float out = computeWaveform();
		advancePhase();
		return out;
	}

	/// Read last computed value without advancing.
	float getCurrentValue() const noexcept { return lastValue; }

private:
	void updatePhaseInc() noexcept
	{
		phaseInc = static_cast<float> (rate / sr);
	}

	void advancePhase() noexcept
	{
		phase += phaseInc;
		if (phase >= 1.0f)
		{
			phase -= 1.0f;
			// Generate new S&H value at the start of each cycle
			std::uniform_real_distribution<float> dist (-1.0f, 1.0f);
			sHoldVal = dist (rng);
		}
	}

	[[nodiscard]] float computeWaveform() noexcept
	{
		const float p = phase + phaseOffset;
		const float wp = p - std::floor (p); // wrapped phase 0..1

		float raw = 0.0f;
		switch (waveform)
		{
			case LFOWaveform::Sine:
				raw = std::sin (wp * 2.0f * std::numbers::pi_v<float>);
				break;

			case LFOWaveform::Triangle:
				raw = (wp < 0.5f) ? (4.0f * wp - 1.0f) : (3.0f - 4.0f * wp);
				break;

			case LFOWaveform::Saw:
				raw = 2.0f * wp - 1.0f;
				break;

			case LFOWaveform::Square:
				raw = (wp < 0.5f) ? 1.0f : -1.0f;
				break;

			case LFOWaveform::SampleAndHold:
				raw = sHoldVal;
				break;
		}

		// Scale and apply bipolar/unipolar
		float out = raw * depth;
		if (!bipolar)
			out = out * 0.5f + depth * 0.5f;

		lastValue = out;
		return out;
	}

	LFOWaveform       waveform    { LFOWaveform::Sine };
	double            sr          { 44100.0 };
	float             rate        { 1.0f };
	float             depth       { 1.0f };
	float             phaseOffset { 0.0f };
	bool              bipolar     { true };

	float             phase       { 0.0f };
	float             phaseInc    { 0.0f };
	float             sHoldVal    { 0.0f };
	float             lastValue   { 0.0f };

	std::minstd_rand  rng;
};

} // namespace bodsp
