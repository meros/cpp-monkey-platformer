// Leafwind -- tiny offline synthesizer: oscillators, sweeps, envelopes, one-pole filters and
// mixing, rendered once into sample buffers (docs/DESIGN.md section 6).
#pragma once

#include <SFML/Config.hpp>

#include <cstdint>
#include <random>
#include <vector>

namespace synth {

constexpr int SR = 44100;
using Buf = std::vector<float>;

enum class Wave { Sine, Tri, Square, Saw, Noise };

struct Rng {
	std::mt19937 g;
	explicit Rng(unsigned s = 1) : g(s) {}
	float operator()(float a, float b) { return std::uniform_real_distribution<float>(a, b)(g); }
};

inline int samples(float sec) { return static_cast<int>(sec * SR); }

// Oscillator with a frequency sweep f0 -> f1 (exponential if expSweep) over dur seconds.
Buf tone(Wave w, float f0, float f1, float dur, bool expSweep = false, float duty = 0.5f, unsigned seed = 1);
Buf noise(float dur, unsigned seed = 1);
Buf pinkNoise(float dur, unsigned seed = 1);
Buf brownNoise(float dur, unsigned seed = 1);

// Envelopes (in place)
void envAD(Buf& b, float attack, float decay, bool expDecay = false);
void envADSR(Buf& b, float a, float d, float s, float r);
void fadeEdges(Buf& b, float sec);

// Filters (in place)
void lowpass(Buf& b, float cutoff);
void lowpassSweep(Buf& b, float c0, float c1);
void highpass(Buf& b, float cutoff);
void bandpass(Buf& b, float lo, float hi);

void gain(Buf& b, float g);
void tremolo(Buf& b, float rate, float depth);
void vibratoPitch(Buf& b, float rate, float depthRatio); // crude resample-based vibrato
void mix(Buf& dst, const Buf& src, float g = 1.f, int offset = 0, bool wrap = false);
Buf concat(const Buf& a, const Buf& b);
Buf reversed(const Buf& b);
void normalize(Buf& b, float peak);
std::vector<sf::Int16> toPCM(const Buf& b);

float noteHz(int midi);

} // namespace synth
