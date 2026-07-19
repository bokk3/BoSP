#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

namespace bodsp
{
class DelayLine
{
public:
	DelayLine() noexcept = default;

	void prepare (double sampleRate, float maxDelaySeconds, int numChannels)
	{
		sr = sampleRate;
		channels = std::max (1, numChannels);
		const int maxDelaySamples = static_cast<int> (std::ceil (maxDelaySeconds * (float) sr));
		bufferSize = std::max (1, maxDelaySamples + 1);

		buffer.assign ((size_t) channels, std::vector<float> ((size_t) bufferSize, 0.0f));
		writeIndex.assign ((size_t) channels, 0u);
	}

	void reset() noexcept
	{
		for (auto &b : buffer)
			std::fill (b.begin(), b.end(), 0.0f);
		std::fill (writeIndex.begin(), writeIndex.end(), 0u);
	}

	void pushSample (int channel, float x) noexcept
	{
		if (channel < 0) return;
		const size_t ch = static_cast<size_t> (std::min (channel, channels - 1));
		buffer[ch][writeIndex[ch]] = x;
		writeIndex[ch] = (writeIndex[ch] + 1) % (size_t) bufferSize;
	}

	float read (int channel, float delaySamples) const noexcept
	{
		if (bufferSize <= 0 || channels <= 0)
			return 0.0f;

		const size_t ch = static_cast<size_t> (std::clamp (channel, 0, channels - 1));

		float readPos = static_cast<float> (writeIndex[ch]) - delaySamples;
		while (readPos < 0.0f)
			readPos += static_cast<float> (bufferSize);
		while (readPos >= static_cast<float> (bufferSize))
			readPos -= static_cast<float> (bufferSize);

		const int i1 = static_cast<int> (std::floor (readPos));
		int idx1 = i1;
		if (idx1 >= bufferSize) idx1 -= bufferSize;
		if (idx1 < 0) idx1 += bufferSize;

		int idx2 = idx1 + 1;
		if (idx2 >= bufferSize) idx2 -= bufferSize;

		const float frac = readPos - static_cast<float> (i1);
		const float s1 = buffer[ch][(size_t) idx1];
		const float s2 = buffer[ch][(size_t) idx2];
		return s1 + (s2 - s1) * frac;
	}

	int getBufferSize() const noexcept { return bufferSize; }
	int getNumChannels() const noexcept { return channels; }
	int getMaxDelaySamples() const noexcept { return bufferSize - 1; }

private:
	double sr { 44100.0 };
	int channels { 0 };
	int bufferSize { 0 };
	std::vector<std::vector<float>> buffer;
	std::vector<size_t> writeIndex;
};

} // namespace bodsp
