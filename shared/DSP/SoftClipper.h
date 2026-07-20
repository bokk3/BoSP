#pragma once

/// bodsp::SoftClipper — Output limiter/saturation with selectable algorithm.
/// Separate from WaveShaper (which is for creative distortion).
/// This is intended for output protection and subtle saturation.
/// Realtime safe: stateless process, no allocations. JUCE-free.

#include <cmath>
#include <numbers>
#include <algorithm>

namespace bodsp
{

class SoftClipper
{
public:
	enum class ClipMode { Tanh, Atan, Cubic, SoftKnee };

	SoftClipper() noexcept = default;

	void setMode (ClipMode m) noexcept { mode = m; }

	/// Pre-gain applied before clipping (dB).
	void setInputGainDb (float db) noexcept
	{
		inputGain = std::pow (10.0f, db / 20.0f);
	}

	/// Post-gain applied after clipping (dB).
	void setOutputGainDb (float db) noexcept
	{
		outputGain = std::pow (10.0f, db / 20.0f);
	}

	/// Knee width for SoftKnee mode [0..1]. 0 = hard clip, 1 = very soft.
	void setKnee (float k) noexcept { knee = std::clamp (k, 0.0f, 1.0f); }

	/// No state needed, kept for API consistency.
	void prepare (double /*sampleRate*/) noexcept {}
	void reset()  noexcept {}

	/// Process one sample. Realtime safe, no allocations.
	[[nodiscard]] float processSample (float x) noexcept
	{
		x *= inputGain;

		float y = 0.0f;
		switch (mode)
		{
			case ClipMode::Tanh:
				y = std::tanh (x);
				break;

			case ClipMode::Atan:
				// Normalized so atan clip peaks at 1.0 for large input
				y = (2.0f / std::numbers::pi_v<float>) * std::atan (x * std::numbers::pi_v<float> * 0.5f);
				break;

			case ClipMode::Cubic:
			{
				// Cubic soft clip: y = 1.5*(x - x^3/3) for |x|<=1, else sign(x)
				// Then normalize peak at x=1: f(1) = 1.5*(1 - 1/3) = 1.0
				const float ax = std::abs (x);
				if (ax < 1.0f)
					y = 1.5f * (x - (x * x * x) / 3.0f);
				else
					y = (x >= 0.0f) ? 1.0f : -1.0f;
				break;
			}

			case ClipMode::SoftKnee:
			{
				// Threshold T: below T, pass through; above T+knee, hard clip at 1.
				// In between: smooth polynomial blend.
				const float T  = 1.0f - knee * 0.5f;
				const float ax = std::abs (x);
				const float sgn = (x >= 0.0f) ? 1.0f : -1.0f;
				if (ax <= T)
				{
					y = x;
				}
				else if (ax >= 1.0f)
				{
					y = sgn;
				}
				else
				{
					// Smooth blend in the knee region [T, 1]
					const float kw = 1.0f - T;
					const float t  = (ax - T) / kw;  // 0..1 in knee
					const float blend = t * t * (3.0f - 2.0f * t); // smoothstep
					y = sgn * (T + kw * blend);
				}
				break;
			}
		}

		return y * outputGain;
	}

private:
	ClipMode mode       { ClipMode::Tanh };
	float    inputGain  { 1.0f };
	float    outputGain { 1.0f };
	float    knee       { 0.5f };
};

} // namespace bodsp
