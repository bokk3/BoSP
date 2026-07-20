#pragma once

/// bodsp::DryWet — Generic dry/wet mixer with equal-power and linear crossfade.
/// Realtime safe: no allocations in process(). JUCE-free.

#include <cmath>
#include <numbers>

namespace bodsp
{

class DryWet
{
public:
	enum class CrossfadeMode { Linear, EqualPower };

	DryWet() noexcept = default;

	/// Set crossfade algorithm.
	void setMode (CrossfadeMode m) noexcept
	{
		mode = m;
		updateCoefficients();
	}

	/// Set wet amount: 0 = fully dry, 1 = fully wet.
	void setMix (float wetAmount) noexcept
	{
		mix = std::clamp (wetAmount, 0.0f, 1.0f);
		updateCoefficients();
	}

	/// No state to initialise, but kept for API consistency.
	void prepare (double /*sampleRate*/) noexcept { updateCoefficients(); }

	/// No state to reset.
	void reset() noexcept {}

	/// Return blended sample. Realtime safe, no allocations.
	[[nodiscard]] float process (float dry, float wet) const noexcept
	{
		return dry * dryGain + wet * wetGain;
	}

	float getMix()     const noexcept { return mix; }
	float getDryGain() const noexcept { return dryGain; }
	float getWetGain() const noexcept { return wetGain; }

private:
	void updateCoefficients() noexcept
	{
		if (mode == CrossfadeMode::EqualPower)
		{
			const float angle = mix * std::numbers::pi_v<float> * 0.5f;
			dryGain = std::cos (angle);
			wetGain = std::sin (angle);
		}
		else
		{
			dryGain = 1.0f - mix;
			wetGain = mix;
		}
	}

	CrossfadeMode mode   { CrossfadeMode::EqualPower };
	float         mix    { 0.5f };
	float         dryGain{ 0.707f };
	float         wetGain{ 0.707f };
};

} // namespace bodsp
