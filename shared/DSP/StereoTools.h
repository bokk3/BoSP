#pragma once

/// bodsp::StereoTools — Stereo imaging and utility processing.
/// Provides width, mono sum, L/R swap, phase inversion, and Mid/Side encode/decode.
/// All static methods are stateless. The class also maintains a stateful mode
/// for use as a processing stage via process(float& L, float& R).
/// Realtime safe: no allocations anywhere. JUCE-free.

#include <algorithm>
#include <cmath>

namespace bodsp

{

class StereoTools
{
public:
	StereoTools() noexcept = default;

	// --- No-op lifecycle (API consistency) ---
	void prepare (double /*sampleRate*/) noexcept {}
	void reset()                         noexcept {}

	// --- Stateful parameters ---
	void setWidth      (float w) noexcept { width      = w; }
	void setMono       (bool b)  noexcept { mono       = b; }
	void setSwapLR     (bool b)  noexcept { swapLR     = b; }
	void setInvertLeft (bool b)  noexcept { invertLeft = b; }
	void setInvertRight(bool b)  noexcept { invertRight= b; }
	void setMidSideMode(bool b)  noexcept { midSide    = b; }

	/// Apply all enabled stateful processing to an L/R pair.
	void process (float& L, float& R) noexcept
	{
		if (swapLR)     swapLRStatic (L, R);
		if (invertLeft) L = -L;
		if (invertRight)R = -R;
		if (mono)       toMonoStatic (L, R);
		if (midSide)
		{
			float M, S;
			encodeMidSide (L, R, M, S);
			L = M; R = S;
		}
		else
		{
			applyWidth (L, R, width);
		}
	}

	// --- Static utility methods (use directly without an instance) ---

	/// Width: 0 = mono, 1 = original, 2 = double width.
	static void applyWidth (float& L, float& R, float w) noexcept
	{
		const float mid  = (L + R) * 0.5f;
		const float side = (L - R) * 0.5f * w;
		L = mid + side;
		R = mid - side;
	}

	/// Sum to mono.
	static void toMonoStatic (float& L, float& R) noexcept
	{
		const float m = (L + R) * 0.5f;
		L = m; R = m;
	}

	/// Swap L and R channels.
	static void swapLRStatic (float& L, float& R) noexcept
	{
		const float tmp = L;
		L = R;
		R = tmp;
	}

	/// Invert left channel phase.
	static void invertLStatic  (float& L) noexcept { L = -L; }

	/// Invert right channel phase.
	static void invertRStatic  (float& R) noexcept { R = -R; }

	/// Mid/Side encode: M = (L+R)/2,  S = (L-R)/2
	static void encodeMidSide (float L, float R, float& M, float& S) noexcept
	{
		M = (L + R) * 0.5f;
		S = (L - R) * 0.5f;
	}

	/// Mid/Side decode: L = M+S,  R = M-S
	static void decodeMidSide (float M, float S, float& L, float& R) noexcept
	{
		L = M + S;
		R = M - S;
	}

private:
	float width       { 1.0f };
	bool  mono        { false };
	bool  swapLR      { false };
	bool  invertLeft  { false };
	bool  invertRight { false };
	bool  midSide     { false };
};

} // namespace bodsp
