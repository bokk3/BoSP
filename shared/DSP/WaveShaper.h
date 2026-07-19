#pragma once

// Simple, header-only waveshaper with multiple modes.
// Initially implements only soft clipping (tanh). Placed in namespace bodsp
// so it can be reused across plugins. No JUCE GUI dependencies.

#include <cmath>

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

	// Process a single sample. Fast, no allocations.
	float process (float x) noexcept
	{
		switch (mode)
		{
			case Mode::Soft:
				// Soft clipping using tanh
				return std::tanh (x);

			case Mode::Tube:
				// Gentle tube-like saturation: x / (1 + |x|)
				return x / (1.0f + std::fabs (x));

			case Mode::Hard:
				// Hard clip
				if (x > 1.0f) return 1.0f;
				if (x < -1.0f) return -1.0f;
				return x;

			case Mode::Foldback:
			{
				// Foldback distortion: reflect waveform back into range when exceeding threshold
				const float thresh = 1.0f;
				float ax = std::fabs (x);
				if (ax <= thresh)
					return x;

				// fold amount
				float excess = ax - thresh;
				float period = 2.0f * thresh;
				float folded = std::fmod (excess, period);
				float out = folded <= thresh ? (thresh - folded) : (folded - thresh);
				return (x < 0.0f) ? -out : out;
			}
		}

		return x;
	}

	void setMode (Mode m) noexcept { mode = m; }

private:
	Mode mode { Mode::Soft };
};

} // namespace bodsp
