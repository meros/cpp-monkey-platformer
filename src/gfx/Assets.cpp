#include "Assets.h"

#include "core/Level.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

float hash1(int x, int y, int seed) {
	unsigned h = static_cast<unsigned>(x) * 374761393u + static_cast<unsigned>(y) * 668265263u +
	             static_cast<unsigned>(seed) * 2246822519u;
	h = (h ^ (h >> 13)) * 1274126177u;
	h ^= h >> 16;
	return (h & 0xFFFFFF) / static_cast<float>(0xFFFFFF);
}

static float smooth(float t) { return t * t * (3.f - 2.f * t); }

float valueNoise(float x, float y, int period, int seed) {
	int x0 = static_cast<int>(std::floor(x)), y0 = static_cast<int>(std::floor(y));
	float fx = smooth(x - x0), fy = smooth(y - y0);
	auto h = [&](int a, int b) {
		a = ((a % period) + period) % period;
		b = ((b % period) + period) % period;
		return hash1(a, b, seed);
	};
	float a = h(x0, y0), b = h(x0 + 1, y0), c = h(x0, y0 + 1), d = h(x0 + 1, y0 + 1);
	return (a + (b - a) * fx) + ((c + (d - c) * fx) - (a + (b - a) * fx)) * fy;
}

static sf::Texture makeNoise() {
	const int N = 128;
	sf::Image img;
	img.create(N, N);
	for (int y = 0; y < N; ++y) {
		for (int x = 0; x < N; ++x) {
			float v = 0.f, amp = 0.5f, tot = 0.f;
			int per = 8;
			for (int o = 0; o < 3; ++o) {
				v += amp * valueNoise(x * per / static_cast<float>(N), y * per / static_cast<float>(N), per, 17 + o);
				tot += amp;
				amp *= 0.5f;
				per *= 2;
			}
			v /= tot;
			// contrast 0.25 around 1.0 (multiply texture)
			float m = 0.75f + 0.25f * std::clamp((v - 0.2f) / 0.6f, 0.f, 1.f);
			sf::Uint8 g = static_cast<sf::Uint8>(m * 255.f);
			img.setPixel(x, y, sf::Color(g, g, static_cast<sf::Uint8>(std::min(255.f, g * 1.02f)), 255));
		}
	}
	sf::Texture t;
	t.loadFromImage(img);
	t.setRepeated(true);
	t.setSmooth(true);
	return t;
}

static sf::Texture makePaper() {
	const int N = 256;
	sf::Image img;
	img.create(N, N);
	for (int y = 0; y < N; ++y) {
		for (int x = 0; x < N; ++x) {
			float n = valueNoise(x / 16.f, y / 16.f, 16, 3) * 0.6f + valueNoise(x / 4.f, y / 4.f, 64, 9) * 0.4f;
			float fiber = hash1(x, y, 5) * 0.5f;
			float v = 0.94f + 0.06f * n - 0.03f * fiber;
			img.setPixel(x, y, sf::Color(static_cast<sf::Uint8>(255 * v), static_cast<sf::Uint8>(244 * v),
			                             static_cast<sf::Uint8>(214 * v)));
		}
	}
	sf::Texture t;
	t.loadFromImage(img);
	t.setRepeated(true);
	t.setSmooth(true);
	return t;
}

static sf::Texture makeVignette() {
	const int W = 200, H = 150;
	sf::Image img;
	img.create(W, H);
	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			float dx = (x + 0.5f) / W * 2.f - 1.f, dy = (y + 0.5f) / H * 2.f - 1.f;
			float d = std::sqrt(dx * dx * 0.9f + dy * dy * 1.1f) / std::sqrt(2.f);
			float a = std::clamp((d - 0.35f) / 0.65f, 0.f, 1.f);
			a = a * a * (3.f - 2.f * a);
			img.setPixel(x, y, sf::Color(255, 255, 255, static_cast<sf::Uint8>(a * 255)));
		}
	}
	sf::Texture t;
	t.loadFromImage(img);
	t.setSmooth(true);
	return t;
}

static sf::Texture makeAtlas() {
	const int C = 64;
	sf::Image img;
	img.create(C * 5, C, sf::Color(255, 255, 255, 0));
	for (int y = 0; y < C; ++y) {
		for (int x = 0; x < C; ++x) {
			float dx = (x + 0.5f) / C * 2.f - 1.f, dy = (y + 0.5f) / C * 2.f - 1.f;
			float r = std::sqrt(dx * dx + dy * dy);
			// glow: smooth radial falloff
			float g = std::clamp(1.f - r, 0.f, 1.f);
			g = g * g;
			img.setPixel(x, y, sf::Color(255, 255, 255, static_cast<sf::Uint8>(g * 255)));
			// leaf: pointed ellipse along x with a vein
			float lx = dx, ly = dy;
			float halfW = 0.55f * std::sqrt(std::max(0.f, 1.f - lx * lx)) * (1.f - 0.2f * lx);
			float leafA = std::clamp((halfW - std::fabs(ly)) * 12.f, 0.f, 1.f);
			float vein = std::fabs(ly) < 0.05f && lx < 0.8f ? 0.75f : 1.f;
			sf::Uint8 lv = static_cast<sf::Uint8>(255 * vein);
			img.setPixel(C + x, y, sf::Color(lv, lv, lv, static_cast<sf::Uint8>(leafA * 255)));
			// star: 4-point sparkle
			float s = std::max(0.f, 1.f - std::fabs(dx) * 7.f - std::fabs(dy) * 1.1f) +
			          std::max(0.f, 1.f - std::fabs(dy) * 7.f - std::fabs(dx) * 1.1f);
			s += std::max(0.f, 0.6f - r * 1.6f);
			s = std::clamp(s, 0.f, 1.f);
			img.setPixel(2 * C + x, y, sf::Color(255, 255, 255, static_cast<sf::Uint8>(s * 255)));
			// solid
			img.setPixel(3 * C + x, y, sf::Color(255, 255, 255, 255));
			// drop: soft-edged disc
			float d = std::clamp((1.f - r) * 6.f, 0.f, 1.f);
			img.setPixel(4 * C + x, y, sf::Color(255, 255, 255, static_cast<sf::Uint8>(d * 255)));
		}
	}
	sf::Texture t;
	t.loadFromImage(img);
	t.setSmooth(true);
	return t;
}

static const char* kGreyShader = R"(
uniform sampler2D tex;
uniform float amount;
void main() {
	vec4 c = texture2D(tex, gl_TexCoord[0].xy);
	float l = dot(c.rgb, vec3(0.299, 0.587, 0.114));
	gl_FragColor = vec4(mix(c.rgb, vec3(l) * vec3(0.95, 0.97, 1.0), amount), c.a) * gl_Color;
}
)";

bool Assets::load() {
	static const char* names[4] = {"stand", "walk", "walk", "jump"};
	bool ok = true;
	for (int a = 0; a < 4; ++a) {
		for (int r = 0; r < 2; ++r) {
			char path[128];
			if (a == 1 || a == 2)
				std::snprintf(path, sizeof(path), "apa_%s_%s_%d.tga", names[a], r ? "right" : "left", a);
			else
				std::snprintf(path, sizeof(path), "apa_%s_%s.tga", names[a], r ? "right" : "left");
			if (!myMonkey[a][r].loadFromFile(dataPath(path))) ok = false;
			myMonkey[a][r].setSmooth(false);
		}
	}
	if (!regular.loadFromFile(dataPath("fonts/DejaVuSans.ttf"))) ok = false;
	if (!bold.loadFromFile(dataPath("fonts/DejaVuSans-Bold.ttf"))) ok = false;
	atlas = makeAtlas();
	noise = makeNoise();
	paper = makePaper();
	vignette = makeVignette();
	shadersOk = sf::Shader::isAvailable() && grey.loadFromMemory(kGreyShader, sf::Shader::Fragment);
	if (shadersOk) grey.setUniform("tex", sf::Shader::CurrentTexture);
	return ok;
}
