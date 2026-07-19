#pragma once

// Simple, header-only waveshaper with multiple modes.
// Initially implements only soft clipping (tanh). Placed in namespace bodsp
// so it can be reused across plugins. No JUCE GUI dependencies.

#include <cmath>
#include <numbers>

namespace bodsp
{
class WaveShaper
{
public:
	enum class Mode
	{
		Soft,
		Tube,
		Hard,
		Foldback
	};

	WaveShaper() noexcept = default;
	void setSoftClipAt0dB (bool v) noexcept { softClip0db = v; }

	// Process a single sample. Fast, no allocations.
	float process (float x) noexcept
	{
		switch (mode)
		{
			case Mode::Soft:
				// Soft clipping: either tanh or a 0dB-threshold soft clip
				if (softClip0db)
				{
					// Soft clip with threshold at 1.0 (0 dB). Smoothly compresses values above 1.0
					const float t = 1.0f;
					const float ax = std::fabs (x);
					if (ax <= t)
						return x;
					const float over = ax - t;
					// map over -> [0, 1) with exponential curve for musical compression
					const float compressed = t + (1.0f - std::exp (-over));
					return x < 0.0f ? -compressed : compressed;
				}
				else
				{
					// Classic soft clipping using tanh scaled for musical response
					return std::tanh (x * 0.8f);
				}

			case Mode::Tube:
				// Tube-like saturation using a smooth asymmetric polynomial approximation.
				// This gives a warmer, asymmetric response compared with tanh.
				{
					const float ax = std::fabs (x);
					const float sign = x < 0.0f ? -1.0f : 1.0f;
					// coefficients chosen for a musical compromise
					const float a = 2.0f;
					const float y = (x * (a + 1.0f)) / (a + ax);
					return y;
				}

			case Mode::Hard:
				// Hard clip to +-1.0f
				if (x > 1.0f) return 1.0f;
				if (x < -1.0f) return -1.0f;
				return x;

			case Mode::Foldback:
			{
				// Foldback distortion: smoother fold using a sin-based transfer
				const float thresh = 1.0f;
				float ax = std::fabs (x);
				if (ax <= thresh)
					return x;

				// Compute folded position in [0, 2*thresh)
				float pos = std::fmod (ax - thresh, 2.0f * thresh);
				// Mirror into [0, thresh]
				float mirrored = pos <= thresh ? pos : (2.0f * thresh - pos);

				// Map to smooth curve using sin to avoid hard discontinuities
				const float mapped = std::sin (std::numbers::pi_v<float> * 0.5f * (mirrored / thresh));
				return (x < 0.0f) ? -mapped : mapped;
			}
		}

		return x;
	}

	void setMode (Mode m) noexcept { mode = m; }

private:
	Mode mode { Mode::Soft };
	bool softClip0db { false };
};

} // namespace bodsp
