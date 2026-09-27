// Leafwind -- sky and parallax layers (docs/DESIGN.md 5.2).
#pragma once

#include "Draw.h"
#include "Palette.h"
#include "core/Level.h"

#include <vector>

class Background {
public:
	void build(Biome biome, float levelWpx, float levelHpx, int seed);
	// Screen-space sky gradient (+ storm clouds / lightning flash), clouds and far..near layers.
	void drawSky(sf::RenderTarget& t, V2 cam, float time, float flash) const;
	void drawLayers(sf::RenderTarget& t, V2 cam, float time) const;
	void drawShafts(sf::RenderTarget& t, V2 cam, float time) const;
	// Foreground foliage over the player (parallax 1.3), avoiding the player's screen position.
	void drawForeground(sf::RenderTarget& t, V2 cam, V2 playerScreen, float time) const;

private:
	struct FgItem {
		float x;       // layer x
		float scale;
		int kind;      // 0 fern, 1 big leaf pair, 2 grass clump
		float phase;
	};
	struct Cloud {
		V2 pos;
		float w;
		int seed;
	};
	float yOffset(V2 cam, float f) const;
	float W_WRAP() const;

	Biome myBiome = Biome::Canopy;
	Palette pal;
	float levelW = 0.f, levelH = 0.f;
	Canvas farBack, farFront, mid, nearL;
	std::vector<FgItem> fg;
	std::vector<Cloud> clouds;
	bool shafts = false;
};
