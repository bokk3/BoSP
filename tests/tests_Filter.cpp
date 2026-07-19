#include <cassert>
#include <cmath>
#include <iostream>
#include "BiquadFilter.h"

int main()
{
	using namespace bodsp;
	const double sr = 44100.0;
	const float  eps = 1e-5f;

	// Helper: process N silent samples
	auto flush = [&] (BiquadFilter& f, int n)
	{
		for (int i = 0; i < n; ++i)
			f.processSample (0, 0.0f);
	};

	// -------------------------------------------------------
	// 1. Lowpass: DC (0 Hz) should pass, Nyquist should attenuate
	// -------------------------------------------------------
	{
		BiquadFilter f;
		f.prepare (sr, 1);
		f.setMode (BiquadFilter::Mode::Lowpass);
		f.setFrequency (500.0f);
		f.setQ (0.707f);

		// Feed a burst of DC (1.0) and measure steady-state output
		float out = 0.0f;
		for (int i = 0; i < 2000; ++i)
			out = f.processSample (0, 1.0f);
		// DC should pass through a lowpass (within a small tolerance)
		assert (std::fabs (out - 1.0f) < 0.01f);

		// Feed Nyquist (+1/-1) alternating — should be attenuated
		float nyquist = 0.0f;
		for (int i = 0; i < 2000; ++i)
			nyquist = f.processSample (0, (i % 2 == 0) ? 1.0f : -1.0f);
		assert (std::fabs (nyquist) < 0.1f);
	}

	// -------------------------------------------------------
	// 2. Highpass: Nyquist passes, DC attenuated
	// -------------------------------------------------------
	{
		BiquadFilter f;
		f.prepare (sr, 1);
		f.setMode (BiquadFilter::Mode::Highpass);
		f.setFrequency (500.0f);
		f.setQ (0.707f);

		float dc = 0.0f;
		for (int i = 0; i < 2000; ++i)
			dc = f.processSample (0, 1.0f);
		// DC should be blocked by highpass
		assert (std::fabs (dc) < 0.01f);

		float nyquist = 0.0f;
		for (int i = 0; i < 2000; ++i)
			nyquist = f.processSample (0, (i % 2 == 0) ? 1.0f : -1.0f);
		// Nyquist should pass through a highpass
		assert (std::fabs (nyquist) > 0.5f);
	}

	// -------------------------------------------------------
	// 3. Notch: signal at centre frequency should be attenuated
	// -------------------------------------------------------
	{
		BiquadFilter f;
		f.prepare (sr, 2);  // test multi-channel
		f.setMode (BiquadFilter::Mode::Notch);
		f.setFrequency (1000.0f);
		f.setQ (5.0f);  // narrow notch

		// Generate 1000 Hz sine and measure magnitude after settling
		const float freq = 1000.0f;
		const float phase_inc = 2.0f * 3.141592653589793f * freq / (float) sr;
		float peak = 0.0f;
		for (int i = 0; i < 4000; ++i)
		{
			const float x = std::sin (phase_inc * (float) i);
			const float y = f.processSample (0, x);
			if (i > 2000)
				peak = std::max (peak, std::fabs (y));
		}
		// Notch at 1000 Hz should attenuate strongly
		assert (peak < 0.1f);
	}

	// -------------------------------------------------------
	// 4. Low Shelf: positive gain lifts DC
	// -------------------------------------------------------
	{
		BiquadFilter f;
		f.prepare (sr, 1);
		f.setMode (BiquadFilter::Mode::LowShelf);
		f.setFrequency (200.0f);
		f.setQ (0.707f);
		f.setGainDb (12.0f);

		float dc = 0.0f;
		for (int i = 0; i < 4000; ++i)
			dc = f.processSample (0, 1.0f);
		// 12 dB boost at DC → output should be ~4x (linear: 10^(12/20) ≈ 3.98)
		assert (dc > 3.5f && dc < 4.5f);
	}

	// -------------------------------------------------------
	// 5. High Shelf: positive gain lifts Nyquist
	// -------------------------------------------------------
	{
		BiquadFilter f;
		f.prepare (sr, 1);
		f.setMode (BiquadFilter::Mode::HighShelf);
		f.setFrequency (10000.0f);
		f.setQ (0.707f);
		f.setGainDb (12.0f);

		// Measure Nyquist-band signal (alternating +1/-1)
		float peak = 0.0f;
		for (int i = 0; i < 4000; ++i)
		{
			float x = (i % 2 == 0) ? 1.0f : -1.0f;
			float y = f.processSample (0, x);
			if (i > 2000) peak = std::max (peak, std::fabs (y));
		}
		// High shelf at Nyquist: output magnitude should exceed 1.0 (boosted)
		assert (peak > 1.5f);
	}

	// -------------------------------------------------------
	// 6. Reset clears state
	// -------------------------------------------------------
	{
		BiquadFilter f;
		f.prepare (sr, 1);
		f.setMode (BiquadFilter::Mode::Lowpass);
		f.setFrequency (500.0f);
		for (int i = 0; i < 100; ++i)
			f.processSample (0, 1.0f);
		f.reset();
		// After reset with 0 input, output should be 0
		assert (std::fabs (f.processSample (0, 0.0f)) < eps);
	}

	std::cout << "All BiquadFilter unit tests passed!\n";
	return 0;
}
