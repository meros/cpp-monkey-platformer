// Leafwind -- pooled particle system (docs/DESIGN.md 5.7). Coordinates in world pixels.
#pragma once

#include "Draw.h"

#include <random>
#include <vector>

enum class PKind { Dot, Leaf, Streak, Star, Drop, Firefly, Ring };

struct Particle {
	V2 pos, vel;
	float life = 0.f, maxLife = 1.f;
	float size = 4.f;
	float rot = 0.f, spin = 0.f;
	float gravity = 0.f;  // px/s^2
	float drag = 0.f;     // 1/s
	float wobble = 0.f;   // leaf sway amplitude (px/s)
	float phase = 0.f;
	sf::Color color;
	PKind kind = PKind::Dot;
	bool additive = false;
	bool fadeIn = false;
	bool shrink = true;
};

class Particles {
public:
	static constexpr size_t kMax = 2000;
	Particles();
	void seed(unsigned s) { rng.seed(s); }
	void clear() { parts.clear(); }
	Particle& spawn();
	void update(float dt, float windX = 0.f);
	// Draw normal particles (alpha blend) then additive ones.
	void draw(sf::RenderTarget& t, const sf::Texture& atlas);
	size_t count() const { return parts.size(); }

	float rnd(float a, float b) { return std::uniform_real_distribution<float>(a, b)(rng); }
	int rndi(int a, int b) { return std::uniform_int_distribution<int>(a, b)(rng); }

	// Presets
	void dust(V2 p, int n, float spread, sf::Color c);
	void sparkles(V2 p, int n, sf::Color c, float life, float speed);
	void leafBurst(V2 p, int n, sf::Color a, sf::Color b);
	void splash(V2 p, int n);
	void spores(V2 p, int n, sf::Color c);
	void pebbles(V2 p, int n, sf::Color c);
	void ring(V2 p, float r0, sf::Color c, float life);

	std::mt19937 rng;

private:
	std::vector<Particle> parts;
	SpriteBatch normal, add;
};
