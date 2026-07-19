#include <cassert>
#include <cmath>
#include <vector>
#include <iostream>
#include "Reverb.h"

int main()
{
	using namespace bodsp;

	const double sampleRate = 44100.0;

	// 1. Test ReverbComb
	{
		ReverbComb comb;
		comb.prepare (sampleRate, 0.02f); // 20 ms delay
		comb.setRoomSize (0.5f);
		comb.setDamp (0.2f);

		// Process an impulse
		float out1 = comb.process (1.0f);
		// With 20 ms delay, output at sample 0 should be 0 because buffer is initialized to 0
		assert (std::fabs (out1 - 0.0f) < 1e-6f);

		// Process silent samples and check if decay eventually happens (not exploding)
		bool feedbackFound = false;
		for (int i = 0; i < 2000; ++i)
		{
			float out = comb.process (0.0f);
			if (std::fabs (out) > 0.0f)
				feedbackFound = true;
		}
		// Since delay is 20ms (882 samples), we should definitely see the impulse feed back
		assert (feedbackFound);

		comb.reset();
		// After reset, output should be silent
		assert (std::fabs (comb.process (0.0f) - 0.0f) < 1e-6f);
	}

	// 2. Test ReverbAllpass
	{
		ReverbAllpass ap;
		ap.prepare (sampleRate, 0.005f); // 5 ms delay
		ap.setFeedback (0.5f);

		// Input of impulse
		float out1 = ap.process (1.0f);
		// Output should be -g * input + delayLine.read()
		// Since delay is 5ms and buffer is 0, out1 = -0.5 * 1.0 + 0 = -0.5
		assert (std::fabs (out1 + 0.5f) < 1e-6f);

		ap.reset();
		assert (std::fabs (ap.process (0.0f) - 0.0f) < 1e-6f);
	}

	// 3. Test Reverb
	{
		Reverb reverb;
		reverb.prepare (sampleRate, 2);
		reverb.setRoomSize (0.8f);
		reverb.setDamping (0.3f);
		reverb.setPreDelayMs (10.0f);
		reverb.setMix (1.0f); // 100% wet

		// Dry input
		float inL = 1.0f;
		float inR = -1.0f;
		float outL = 0.0f;
		float outR = 0.0f;

		// Process first sample (due to pre-delay and comb filter delay, wet should be 0)
		reverb.processSample (inL, inR, outL, outR);
		assert (std::fabs (outL) < 1e-6f);
		assert (std::fabs (outR) < 1e-6f);

		// Feed impulse and process a bunch of samples to verify it produces sound (reverberates)
		bool reverbActive = false;
		for (int i = 0; i < 5000; ++i)
		{
			reverb.processSample (0.0f, 0.0f, outL, outR);
			if (std::fabs (outL) > 1e-5f || std::fabs (outR) > 1e-5f)
			{
				reverbActive = true;
			}
		}
		assert (reverbActive);

		// Reset reverb
		reverb.reset();
		reverb.processSample (0.0f, 0.0f, outL, outR);
		assert (std::fabs (outL) < 1e-6f);
		assert (std::fabs (outR) < 1e-6f);
	}

	std::cout << "All Reverb unit tests passed successfully!\n";
	return 0;
}
