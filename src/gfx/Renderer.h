// Leafwind -- draws a World (docs/DESIGN.md section 5). Only this layer touches graphics; it
// reads simulation state and consumes the world's events for particles and screen shake.
#pragma once

#include "Assets.h"
#include "Background.h"
#include "Camera.h"
#include "Particles.h"
#include "TerrainRenderer.h"
#include "core/World.h"

#include <vector>

class Renderer {
public:
	explicit Renderer(const Assets& a) : assets(a) {}

	void setLevel(const LevelData& L, int seed);
	void resetCamera(const World& w) { cam.reset(w); }
	// Advance camera, particles and ambient effects; consumes w.events.
	void update(World& w, float dt, float time);
	// Draw the whole playfield (sky .. vignette) into t using the 800x600 design view.
	void drawWorld(sf::RenderTarget& t, const World& w, float time);

	// Screen position (design px) of a world point in metres.
	V2 toScreen(b2Vec2 m) const;

	Camera cam;
	Particles parts;
	bool hidePlayer = false;
	bool showSignBubbles = true;
	float lightning = 0.f; // 0..1 flash
	bool thunder = false;  // set when a lightning strike starts; cleared by the game
	int nearSign = -1;     // sign index within bubble range (for UI)
	float signAlpha = 0.f;

private:
	void handleEvents(World& w);
	void ambient(World& w, float dt, float time);
	void buildWater(const LevelData& L);
	void drawWaterBack(sf::RenderTarget& t, float time) const;
	void drawWaterFront(sf::RenderTarget& t, float time) const;
	void drawDecor(Canvas& c, SpriteBatch& glow, const World& w, float time) const;
	void drawObjects(Canvas& c, SpriteBatch& glow, const World& w, float time) const;
	void drawItems(Canvas& c, SpriteBatch& glow, const World& w, float time) const;
	void drawPlayer(sf::RenderTarget& t, const World& w, float time) const;
	void drawRain(sf::RenderTarget& t, const World& w, float time) const;

	const Assets& assets;
	const LevelData* level = nullptr;
	Palette pal;
	Background bg;
	TerrainRenderer terrain;
	struct WaterCol {
		int x, top, bottom; // tiles, inclusive rows
		uint8_t flowTop;
	};
	std::vector<WaterCol> water;
	std::vector<std::pair<int, int>> surfaces; // (x, row) of surface tiles
	float gustTimer = 0.f;
	float nextLightning = 14.f;
	float lightningT = -1.f;
	float spawnAcc[6] = {0, 0, 0, 0, 0, 0};
};
