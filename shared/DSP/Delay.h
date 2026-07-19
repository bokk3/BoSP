#pragma once

// Basic multifunctional delay with auto-ducking (input-driven), band-limited
// feedback (HP + LP), and mono/stereo/ping-pong modes. Header-only, JUCE-free.

#include "DelayLine.h"
#include "OnePoleFilters.h"
#include <vector>
#include <cmath>
#include <algorithm>
#include <atomic>

namespace bodsp
{
class Delay
{
public:
	enum class Mode { Mono, Stereo, PingPong };

	Delay() noexcept = default;

	void prepare (double sampleRate, int numChannels, float maxDelaySeconds = 5.0f)
	{
		sr = sampleRate;
		channels = std::max (1, numChannels);
		delayLine.prepare (sr, maxDelaySeconds, channels);
		lp.prepare (sr, channels);
		hp.prepare (sr, channels);
		// sensible defaults
		setTimeMs (500.0f);
		setFeedback (0.45f);
		setMix (50.0f);
		lp.setCutoffHz (8000.0f);
		hp.setCutoffHz (20.0f);
		duckEnable = true;
		setDuckParams (-18.0f, 4.0f, -18.0f, 10.0f, 200.0f);
		duckEnv = 0.0f;
		reset();
	}

	void reset() noexcept
	{
		delayLine.reset();
		lp.reset();
		hp.reset();
		duckEnv = 0.0f;
	}

	void setTimeMs (float ms) noexcept { timeMs = std::clamp (ms, 0.1f, maxTimeMs()); }
	void setMode (Mode m) noexcept { mode = m; }
	void setFeedback (float f) noexcept { feedback = std::clamp (f, 0.0f, 0.99f); }
	void setMix (float percent) noexcept { mix = std::clamp (percent * 0.01f, 0.0f, 1.0f); }
	void setLP (float hz) noexcept { lp.setCutoffHz (hz); }
	void setHP (float hz) noexcept { hp.setCutoffHz (hz); }

	void setDuckParams (float thresholdDb, float ratioVal, float depthDb, float attackMs, float releaseMs) noexcept
	{
		duckThresholdDb = thresholdDb;
		duckRatio = std::max (1.0f, ratioVal);
		duckDepthDb = std::min (0.0f, depthDb);
		duckAttackMs = std::max (0.01f, attackMs);
		duckReleaseMs = std::max (1.0f, releaseMs);
		updateDuckCoefs();
	}

	void setDuckEnable (bool e) noexcept { duckEnable = e; }

	// Process a single sample. caller should supply 'inputLevel' as the measured
	// input envelope (e.g. max abs across channels) when available; otherwise
	// pass 0 and the function will use the input magnitude of this channel.
	float processSample (int channel, float input, float inputLevel = 0.0f) noexcept
	{
		if (channels <= 0)
			return input;

		const int ch = std::clamp (channel, 0, channels - 1);

		// measure input level (use provided global inputLevel if > 0, else channel magnitude)
		const float measured = (inputLevel > 0.0f) ? inputLevel : std::fabs (input);
		updateDuckEnvelope (measured);

		// read wet sample from delay (before writing current sample)
		const float delaySamples = timeMs * static_cast<float> (sr) * 0.001f;
		float wet = delayLine.read (ch, delaySamples);

		// feedback path
		float fbGain = feedback;
		if (duckEnable)
		{
			const float duckGain = computeDuckGainLinear();
			// apply duck to both wet and feedback to reduce repeats
			wet *= duckGain;
			fbGain *= duckGain;
		}

		float fbSample = wet * fbGain;
		// filter feedback: HP then LP
		float fbFiltered = lp.processSample (ch, hp.processSample (ch, fbSample));

		// determine write channel (pingpong cross-feed)
		int writeCh = ch;
		if (mode == Mode::PingPong && channels > 1)
			writeCh = ch ^ 1;

		// write current input + feedback into delay
		const float toWrite = input + fbFiltered;
		delayLine.pushSample (writeCh, toWrite);

		// mix wet/dry
		const float out = input * (1.0f - mix) + wet * mix;
		return out;
	}

	float getMix() const noexcept { return mix; }
	float getFeedback() const noexcept { return feedback; }

private:
	void updateDuckCoefs() noexcept
	{
		if (sr <= 0.0) return;
		const float dt = 1.0f / static_cast<float> (sr);
		duckAttackCoef = std::exp (-dt / (duckAttackMs * 0.001f));
		duckReleaseCoef = std::exp (-dt / (duckReleaseMs * 0.001f));
	}

	void updateDuckEnvelope (float measured) noexcept
	{
		if (!duckEnable)
			return;

		if (measured > duckEnv)
			duckEnv = duckAttackCoef * duckEnv + (1.0f - duckAttackCoef) * measured;
		else
			duckEnv = duckReleaseCoef * duckEnv + (1.0f - duckReleaseCoef) * measured;
	}

	float linearToDb (float x) const noexcept
	{
		const float eps = 1e-12f;
		return 20.0f * std::log10 (std::max (x, eps));
	}

	float dbToLinear (float db) const noexcept
	{
		return std::pow (10.0f, db * 0.05f);
	}

	float computeDuckGainLinear() const noexcept
	{
		if (!duckEnable)
			return 1.0f;
		const float envDb = linearToDb (duckEnv + 1e-12f);
		if (envDb <= duckThresholdDb)
			return 1.0f;

		const float over = envDb - duckThresholdDb;
		// simple soft ratio: attenuation_dB = over * (1 - 1/ratio)
		const float attenuationDb = std::min (duckDepthDb, over * (1.0f - 1.0f / duckRatio));
		return dbToLinear (-attenuationDb);
	}

	float maxTimeMs() const noexcept { return static_cast<float> (delayLine.getMaxDelaySamples()) * 1000.0f / static_cast<float> (sr); }

	double sr { 44100.0 };
	int channels { 0 };
	DelayLine delayLine;
	OnePoleLowpass lp;
	OnePoleHighpass hp;

	float timeMs { 500.0f };
	Mode mode { Mode::Stereo };
	float feedback { 0.45f };
	float mix { 0.5f };

	// Ducking
	bool duckEnable { true };
	float duckThresholdDb { -18.0f };
	float duckRatio { 4.0f };
	float duckDepthDb { -18.0f };
	float duckAttackMs { 10.0f };
	float duckReleaseMs { 200.0f };
	float duckEnv { 0.0f };
	float duckAttackCoef { 0.0f };
	float duckReleaseCoef { 0.0f };
};

} // namespace bodsp
