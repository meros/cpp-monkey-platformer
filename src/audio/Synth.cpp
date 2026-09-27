#include "Synth.h"

#include <algorithm>
#include <cmath>

namespace synth {

static const float TWO_PI = 6.28318530718f;

float noteHz(int midi) { return 440.f * std::pow(2.f, (midi - 69) / 12.f); }

Buf tone(Wave w, float f0, float f1, float dur, bool expSweep, float duty, unsigned seed) {
	int n = samples(dur);
	Buf b(n);
	Rng r(seed);
	float phase = 0.f;
	for (int i = 0; i < n; ++i) {
		float t = n > 1 ? static_cast<float>(i) / (n - 1) : 0.f;
		float f = expSweep ? f0 * std::pow(f1 / f0, t) : f0 + (f1 - f0) * t;
		phase += f / SR;
		phase -= std::floor(phase);
		float v = 0.f;
		switch (w) {
		case Wave::Sine: v = std::sin(phase * TWO_PI); break;
		case Wave::Tri: v = 4.f * std::fabs(phase - 0.5f) - 1.f; break;
		case Wave::Square: v = phase < duty ? 1.f : -1.f; break;
		case Wave::Saw: v = 2.f * phase - 1.f; break;
		case Wave::Noise: v = r(-1.f, 1.f); break;
		}
		b[i] = v;
	}
	return b;
}

Buf noise(float dur, unsigned seed) { return tone(Wave::Noise, 0, 0, dur, false, 0.5f, seed); }

Buf pinkNoise(float dur, unsigned seed) {
	Buf b = noise(dur, seed);
	float b0 = 0, b1 = 0, b2 = 0;
	for (float& x : b) {
		b0 = 0.99765f * b0 + x * 0.0990460f;
		b1 = 0.96300f * b1 + x * 0.2965164f;
		b2 = 0.57000f * b2 + x * 1.0526913f;
		x = (b0 + b1 + b2 + x * 0.1848f) * 0.25f;
	}
	return b;
}

Buf brownNoise(float dur, unsigned seed) {
	Buf b = noise(dur, seed);
	float acc = 0.f;
	for (float& x : b) {
		acc = (acc + 0.02f * x) / 1.02f;
		x = acc * 3.5f;
	}
	return b;
}

void envAD(Buf& b, float a, float d, bool expDecay) {
	int na = std::max(1, samples(a)), nd = std::max(1, samples(d));
	for (size_t i = 0; i < b.size(); ++i) {
		float e;
		if (static_cast<int>(i) < na) e = static_cast<float>(i) / na;
		else {
			float t = static_cast<float>(i - na) / nd;
			e = expDecay ? std::exp(-5.f * t) : std::max(0.f, 1.f - t);
		}
		b[i] *= e;
	}
}

void envADSR(Buf& b, float a, float d, float s, float r) {
	int n = static_cast<int>(b.size());
	int na = std::max(1, samples(a)), nd = std::max(1, samples(d)), nr = std::max(1, samples(r));
	for (int i = 0; i < n; ++i) {
		float e;
		if (i < na) e = static_cast<float>(i) / na;
		else if (i < na + nd) e = 1.f - (1.f - s) * (i - na) / nd;
		else e = s;
		if (i > n - nr) e *= static_cast<float>(n - i) / nr;
		b[i] *= e;
	}
}

void fadeEdges(Buf& b, float sec) {
	int n = std::min(static_cast<int>(b.size()) / 2, samples(sec));
	for (int i = 0; i < n; ++i) {
		float e = static_cast<float>(i) / n;
		b[i] *= e;
		b[b.size() - 1 - i] *= e;
	}
}

void lowpass(Buf& b, float cutoff) {
	float a = 1.f - std::exp(-TWO_PI * cutoff / SR);
	float y = 0.f;
	for (float& x : b) {
		y += a * (x - y);
		x = y;
	}
}

void lowpassSweep(Buf& b, float c0, float c1) {
	float y = 0.f;
	for (size_t i = 0; i < b.size(); ++i) {
		float t = static_cast<float>(i) / std::max<size_t>(1, b.size() - 1);
		float c = c0 * std::pow(c1 / c0, t);
		float a = 1.f - std::exp(-TWO_PI * c / SR);
		y += a * (b[i] - y);
		b[i] = y;
	}
}

void highpass(Buf& b, float cutoff) {
	Buf lp = b;
	lowpass(lp, cutoff);
	for (size_t i = 0; i < b.size(); ++i) b[i] -= lp[i];
}

void bandpass(Buf& b, float lo, float hi) {
	highpass(b, lo);
	lowpass(b, hi);
}

void gain(Buf& b, float g) {
	for (float& x : b) x *= g;
}

void tremolo(Buf& b, float rate, float depth) {
	for (size_t i = 0; i < b.size(); ++i)
		b[i] *= 1.f - depth * 0.5f * (1.f + std::sin(TWO_PI * rate * i / SR));
}

void vibratoPitch(Buf& b, float rate, float depthRatio) {
	Buf out(b.size());
	float pos = 0.f;
	for (size_t i = 0; i < b.size(); ++i) {
		float speed = 1.f + depthRatio * std::sin(TWO_PI * rate * i / SR);
		size_t p0 = static_cast<size_t>(pos);
		if (p0 + 1 >= b.size()) break;
		float f = pos - p0;
		out[i] = b[p0] * (1.f - f) + b[p0 + 1] * f;
		pos += speed;
	}
	b.swap(out);
}

void mix(Buf& dst, const Buf& src, float g, int offset, bool wrap) {
	for (size_t i = 0; i < src.size(); ++i) {
		long j = offset + static_cast<long>(i);
		if (wrap) j %= static_cast<long>(dst.size());
		if (j < 0) continue;
		if (j >= static_cast<long>(dst.size())) {
			if (!wrap) dst.resize(j + 1, 0.f);
		}
		dst[j] += src[i] * g;
	}
}

Buf concat(const Buf& a, const Buf& b) {
	Buf r = a;
	r.insert(r.end(), b.begin(), b.end());
	return r;
}

Buf reversed(const Buf& b) { return Buf(b.rbegin(), b.rend()); }

void normalize(Buf& b, float peak) {
	float m = 0.f;
	for (float x : b) m = std::max(m, std::fabs(x));
	if (m > 1e-6f) gain(b, peak / m);
}

std::vector<sf::Int16> toPCM(const Buf& b) {
	std::vector<sf::Int16> out(b.size());
	for (size_t i = 0; i < b.size(); ++i) {
		float x = std::tanh(b[i]); // gentle soft clip (unity gain for quiet signals)
		out[i] = static_cast<sf::Int16>(std::clamp(x, -1.f, 1.f) * 32000.f);
	}
	return out;
}

} // namespace synth
