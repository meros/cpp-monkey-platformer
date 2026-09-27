// Leafwind -- the only loaded assets (monkey frames, fonts) plus the handful of textures that
// are generated at startup (docs/DESIGN.md 5, 9).
#pragma once

#include "Draw.h"
#include "core/Player.h"

#include <SFML/Graphics.hpp>

class Assets {
public:
	bool load(); // false if a required file is missing

	const sf::Texture& monkey(PAnim a, bool right) const { return myMonkey[static_cast<int>(a)][right ? 1 : 0]; }

	sf::Font regular, bold;
	sf::Texture atlas;   // glow | leaf | star | solid | drop  (64x64 cells)
	sf::Texture noise;   // 128x128 tileable value noise, values ~0.75..1.0
	sf::Texture paper;   // 256x256 cream paper
	sf::Texture vignette;
	bool shadersOk = false;
	sf::Shader grey;     // desaturation post effect

private:
	sf::Texture myMonkey[4][2];
};

// Deterministic hash noise helpers shared by the renderer.
float hash1(int x, int y = 0, int seed = 0);
float valueNoise(float x, float y, int period, int seed);
