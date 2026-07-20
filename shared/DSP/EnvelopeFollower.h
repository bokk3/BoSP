#pragma once

/// bodsp::EnvelopeFollower — Peak and RMS envelope detector with ballistic attack/release.
/// Powers compressors, gates, transient shapers, ducking, auto-wah.
/// Realtime safe: no allocations in processSample(). JUCE-free.

#include <cmath>
#include <algorithm>

namespace bodsp
{

class EnvelopeFollower
{
public:
	enum class FollowerMode { Peak, RMS };

	EnvelopeFollower() noexcept = default;

	void prepare (double sampleRate) noexcept
	{
		sr = sampleRate;
		updateCoefficients();
		reset();
	}

	void setMode (FollowerMode m) noexcept { mode = m; reset(); }

	/// Attack time in milliseconds.
	void setAttackMs (float ms) noexcept
	{
		attackMs = std::max (0.01f, ms);
		updateCoefficients();
	}

	/// Release time in milliseconds.
	void setReleaseMs (float ms) noexcept
	{
		releaseMs = std::max (0.01f, ms);
		updateCoefficients();
	}

	void reset() noexcept
	{
		envelope = 0.0f;
		meanSquare = 0.0f;
	}

	/// Process one sample and return the current envelope level (linear).
	[[nodiscard]] float processSample (float x) noexcept
	{
		if (mode == FollowerMode::Peak)
		{
			const float abs_x = std::abs (x);
			const float coeff  = (abs_x > envelope) ? attackCoeff : releaseCoeff;
			envelope = coeff * envelope + (1.0f - coeff) * abs_x;
		}
		else // RMS
		{
			const float sq     = x * x;
			const float coeff  = (sq > meanSquare) ? attackCoeff : releaseCoeff;
			meanSquare = coeff * meanSquare + (1.0f - coeff) * sq;
			envelope   = std::sqrt (std::max (0.0f, meanSquare));
		}
		return envelope;
	}

	float getLevel()   const noexcept { return envelope; }
	float getLevelDb() const noexcept
	{
		return (envelope > 1e-9f) ? 20.0f * std::log10 (envelope) : -180.0f;
	}

private:
	void updateCoefficients() noexcept
	{
		// coeff close to 1 = slow, close to 0 = fast
		attackCoeff  = std::exp (-1.0f / (attackMs  * 0.001f * static_cast<float> (sr)));
		releaseCoeff = std::exp (-1.0f / (releaseMs * 0.001f * static_cast<float> (sr)));
	}

	FollowerMode mode        { FollowerMode::Peak };
	double       sr          { 44100.0 };

	float attackMs    { 10.0f };
	float releaseMs   { 100.0f };
	float attackCoeff { 0.99f };
	float releaseCoeff{ 0.999f };

	float envelope   { 0.0f };
	float meanSquare { 0.0f };
};

} // namespace bodsp
