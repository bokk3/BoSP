#pragma once

#include "DelayLine.h"
#include "OnePoleFilters.h"
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

namespace bodsp
{

class ReverbComb
{
public:
	ReverbComb() noexcept = default;

	void prepare (double sampleRate, float delaySeconds)
	{
		sr = sampleRate;
		// Prepare delay line with safety headroom
		delayLine.prepare (sr, delaySeconds * 1.5f, 1);
		delaySamples = delaySeconds * static_cast<float> (sr);
		reset();
	}

	void reset() noexcept
	{
		delayLine.reset();
		filterState = 0.0f;
	}

	void setRoomSize (float size) noexcept { roomSize = size; }
	void setDamp (float damping) noexcept { damp = damping; }

	float process (float input) noexcept
	{
		const float output = delayLine.read (0, delaySamples);
		
		// Damping feedback filter matching Freeverb logic
		// DC feedback gain is kept constant (roomSize) across damping settings
		const float damp1 = damp * 0.4f;
		const float damp2 = roomSize * (1.0f - damp1);
		filterState = (output * damp2) + (filterState * damp1);

		delayLine.pushSample (0, input + filterState);
		return output;
	}

private:
	double sr { 44100.0 };
	DelayLine delayLine;
	float delaySamples { 0.0f };
	float roomSize { 0.5f };
	float damp { 0.5f };
	float filterState { 0.0f };
};

class ReverbAllpass
{
public:
	ReverbAllpass() noexcept = default;

	void prepare (double sampleRate, float delaySeconds)
	{
		sr = sampleRate;
		// Prepare delay line with safety headroom
		delayLine.prepare (sr, delaySeconds * 1.5f, 1);
		delaySamples = delaySeconds * static_cast<float> (sr);
		reset();
	}

	void reset() noexcept
	{
		delayLine.reset();
	}

	void setFeedback (float fb) noexcept { feedback = fb; }

	float process (float input) noexcept
	{
		const float delayOutput = delayLine.read (0, delaySamples);
		
		// Canonical all-pass filter structure:
		// v[n] = x[n] + g * v[n - D]
		// y[n] = -g * v[n] + v[n - D]
		const float temp = input + feedback * delayOutput;
		const float output = -feedback * temp + delayOutput;

		delayLine.pushSample (0, temp);
		return output;
	}

private:
	double sr { 44100.0 };
	DelayLine delayLine;
	float delaySamples { 0.0f };
	float feedback { 0.5f };
};

class Reverb
{
public:
	Reverb() noexcept = default;

	void prepare (double sampleRate, int numChannels)
	{
		sr = sampleRate;
		channels = std::max (1, numChannels);

		// Initialize pre-delay
		preDelayLine.prepare (sr, 0.3f, channels);

		// Left channel delay times (at 44.1 kHz in samples)
		const std::array<float, 8> leftCombTimes = { 1116.0f, 1188.0f, 1277.0f, 1356.0f, 1422.0f, 1491.0f, 1557.0f, 1617.0f };
		const std::array<float, 4> leftApTimes   = { 556.0f, 441.0f, 341.0f, 225.0f };

		// Setup Left channel DSP
		for (size_t i = 0; i < 8; ++i)
			leftCombs[i].prepare (sr, leftCombTimes[i] / 44100.0f);
		for (size_t i = 0; i < 4; ++i)
		{
			leftAllpasses[i].prepare (sr, leftApTimes[i] / 44100.0f);
			leftAllpasses[i].setFeedback (0.5f);
		}

		// Setup Right channel DSP (if we have at least 2 channels, offset by 23 samples)
		for (size_t i = 0; i < 8; ++i)
			rightCombs[i].prepare (sr, (leftCombTimes[i] + 23.0f) / 44100.0f);
		for (size_t i = 0; i < 4; ++i)
		{
			rightAllpasses[i].prepare (sr, (leftApTimes[i] + 23.0f) / 44100.0f);
			rightAllpasses[i].setFeedback (0.5f);
		}

		// Setup Lowpass and Highpass filters for Left and Right channels
		lpLeft.prepare (sr, 1);
		lpRight.prepare (sr, 1);
		hpLeft.prepare (sr, 1);
		hpRight.prepare (sr, 1);

		// Set default parameters
		setRoomSize (0.5f);
		setDamping (0.5f);
		setWidth (1.0f);
		setMix (0.3f);
		setPreDelayMs (10.0f);
		setLPCutoff (15000.0f);
		setHPCutoff (20.0f);

		reset();
	}

	void reset() noexcept
	{
		preDelayLine.reset();
		for (auto& c : leftCombs) c.reset();
		for (auto& c : rightCombs) c.reset();
		for (auto& a : leftAllpasses) a.reset();
		for (auto& a : rightAllpasses) a.reset();
		lpLeft.reset();
		lpRight.reset();
		hpLeft.reset();
		hpRight.reset();
	}

	void setRoomSize (float size) noexcept
	{
		roomSize = std::clamp (size, 0.0f, 0.98f);
		for (auto& c : leftCombs) c.setRoomSize (roomSize);
		for (auto& c : rightCombs) c.setRoomSize (roomSize);
	}

	void setDamping (float damp) noexcept
	{
		damping = std::clamp (damp, 0.0f, 1.0f);
		for (auto& c : leftCombs) c.setDamp (damping);
		for (auto& c : rightCombs) c.setDamp (damping);
	}

	void setWidth (float w) noexcept { width = std::clamp (w, 0.0f, 1.0f); }
	void setMix (float m) noexcept { mix = std::clamp (m, 0.0f, 1.0f); }
	void setPreDelayMs (float ms) noexcept { preDelayMs = std::clamp (ms, 0.0f, 200.0f); }
	void setLPCutoff (float hz) noexcept
	{
		lpLeft.setCutoffHz (hz);
		lpRight.setCutoffHz (hz);
	}
	void setHPCutoff (float hz) noexcept
	{
		hpLeft.setCutoffHz (hz);
		hpRight.setCutoffHz (hz);
	}

	// Process a single stereo/mono sample
	// inputL and inputR are dry input samples.
	// outL and outR are mixed outputs.
	void processSample (float inputL, float inputR, float& outL, float& outR) noexcept
	{
		if (channels <= 0)
		{
			outL = inputL;
			outR = inputR;
			return;
		}

		// 1. Push to pre-delay line
		preDelayLine.pushSample (0, inputL);
		if (channels > 1)
			preDelayLine.pushSample (1, inputR);

		// 2. Read from pre-delay line
		const float preDelaySamples = preDelayMs * static_cast<float> (sr) * 0.001f;
		const float preDelayedL = preDelayLine.read (0, preDelaySamples);
		const float preDelayedR = (channels > 1) ? preDelayLine.read (1, preDelaySamples) : preDelayedL;

		// Input scaling/attenuation for reverb path (standard Freeverb scale)
		const float reverbInputScale = 0.015f;
		const float dryL = inputL;
		const float dryR = inputR;

		// Sum input to mono for comb filters if desired, or process stereo
		const float wetInL = preDelayedL * reverbInputScale;
		const float wetInR = preDelayedR * reverbInputScale;

		// 3. Process parallel comb filters
		float combSumL = 0.0f;
		float combSumR = 0.0f;
		for (size_t i = 0; i < 8; ++i)
		{
			combSumL += leftCombs[i].process (wetInL);
			combSumR += rightCombs[i].process (wetInR);
		}

		// 4. Process series allpass filters
		float apOutputL = combSumL;
		float apOutputR = combSumR;
		for (size_t i = 0; i < 4; ++i)
		{
			apOutputL = leftAllpasses[i].process (apOutputL);
			apOutputR = rightAllpasses[i].process (apOutputR);
		}

		// 5. Apply LP and HP post-filters to the wet path
		float filteredWetL = hpLeft.processSample (0, lpLeft.processSample (0, apOutputL));
		float filteredWetR = hpRight.processSample (0, lpRight.processSample (0, apOutputR));

		// 6. Apply stereo width cross-mix
		const float wet1 = width * 0.5f + 0.5f;
		const float wet2 = 1.0f - wet1;

		// Scale up the wet path to compensate for the 0.015 input attenuation.
		const float wetGainCompensation = 8.0f;
		const float finalWetL = (filteredWetL * wet1 + filteredWetR * wet2) * wetGainCompensation;
		const float finalWetR = (filteredWetR * wet1 + filteredWetL * wet2) * wetGainCompensation;

		// 7. Mix dry/wet
		outL = dryL * (1.0f - mix) + finalWetL * mix;
		outR = dryR * (1.0f - mix) + finalWetR * mix;
	}

	float getRoomSize() const noexcept { return roomSize; }
	float getDamping() const noexcept { return damping; }
	float getWidth() const noexcept { return width; }
	float getMix() const noexcept { return mix; }
	float getPreDelayMs() const noexcept { return preDelayMs; }

private:
	double sr { 44100.0 };
	int channels { 0 };

	DelayLine preDelayLine;
	float preDelayMs { 10.0f };

	std::array<ReverbComb, 8> leftCombs;
	std::array<ReverbComb, 8> rightCombs;

	std::array<ReverbAllpass, 4> leftAllpasses;
	std::array<ReverbAllpass, 4> rightAllpasses;

	OnePoleLowpass lpLeft;
	OnePoleLowpass lpRight;
	OnePoleHighpass hpLeft;
	OnePoleHighpass hpRight;

	float roomSize { 0.5f };
	float damping { 0.5f };
	float width { 1.0f };
	float mix { 0.3f };
};

} // namespace bodsp
