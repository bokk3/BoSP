#include <cassert>
#include <cmath>
#include "WaveShaper.h"

int main()
{
	using namespace bodsp;
	WaveShaper ws;

	// Soft mode: tanh(0) == 0
	ws.setMode(WaveShaper::Mode::Soft);
	assert(std::fabs(ws.process(0.0f) - 0.0f) < 1e-6f);

	// Soft: tanh is odd -> tanh(x) == -tanh(-x)
	float a = 0.5f;
	assert(std::fabs(ws.process(a) + ws.process(-a)) < 1e-6f);

	// Tube mode: should be less than input magnitude for large inputs
	ws.setMode(WaveShaper::Mode::Tube);
	float outTube = ws.process(5.0f);
	assert(std::fabs(outTube) < 5.0f);

	// Hard mode: clamps to +-1
	ws.setMode(WaveShaper::Mode::Hard);
	assert(ws.process(2.0f) == 1.0f);
	assert(ws.process(-2.0f) == -1.0f);

	// Foldback: value exceeding 1.0f should be folded into range
	ws.setMode(WaveShaper::Mode::Foldback);
	float folded = ws.process(1.5f);
	assert(std::fabs(folded) <= 1.0f + 1e-6f);

	return 0;
}
