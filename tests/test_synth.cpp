// Leafwind -- the offline synthesizer produces sane, bounded buffers (docs/DESIGN.md 6).
#include "audio/Synth.h"

#include <cmath>
#include <cstdio>

using namespace synth;

static int fails = 0;
static void check(bool c, const char* what) {
	if (!c) {
		std::printf("  FAIL: %s\n", what);
		++fails;
	}
}

static float peak(const Buf& b) {
	float m = 0.f;
	for (float x : b) m = std::fmax(m, std::fabs(x));
	return m;
}

int main() {
	Buf s = tone(Wave::Sine, 440.f, 440.f, 0.5f);
	check(static_cast<int>(s.size()) == samples(0.5f), "tone length");
	check(std::fabs(peak(s) - 1.f) < 0.01f, "sine peak ~1");
	// frequency check by zero crossings: 440 Hz -> ~440 rising crossings per second
	int crossings = 0;
	for (size_t i = 1; i < s.size(); ++i)
		if (s[i - 1] < 0.f && s[i] >= 0.f) ++crossings;
	check(crossings > 215 && crossings < 225, "sine frequency");
	Buf n = noise(0.2f, 3);
	float before = peak(n);
	lowpass(n, 300.f);
	check(peak(n) < before, "low-pass lowers the peak");
	Buf e = tone(Wave::Square, 220.f, 440.f, 0.09f, false, 0.4f);
	envAD(e, 0.002f, 0.08f);
	check(std::fabs(e.back()) < 0.05f, "decay ends near silence");
	Buf mixd;
	mix(mixd, e, 1.f, 100);
	check(mixd.size() == e.size() + 100, "mix extends destination");
	auto pcm = toPCM(Buf{0.f, 0.5f, 2.f, -3.f});
	check(pcm[0] == 0 && pcm[1] > 10000 && pcm[2] <= 32000 && pcm[3] >= -32000, "PCM conversion clips softly");
	for (float x : pinkNoise(0.5f, 7)) check(std::isfinite(x), "pink noise finite");
	check(std::fabs(noteHz(69) - 440.f) < 0.01f, "A4 = 440 Hz");
	std::printf(fails ? "%d synth check(s) failed\n" : "All synth tests passed.\n", fails);
	return fails ? 1 : 0;
}
