// Leafwind -- terrain rendered once per level into 32x32-tile render-texture chunks, plus the
// few things recomputed per frame (swaying grass blades, roots, glowing moss) (DESIGN 5.3).
#pragma once

#include "Assets.h"
#include "Draw.h"
#include "Palette.h"
#include "core/Level.h"

#include <memory>
#include <vector>

class TerrainRenderer {
public:
	void build(const LevelData& L, const Assets& assets, int seed);
	void drawStatic(sf::RenderTarget& t, const sf::FloatRect& view) const;
	void drawDynamic(sf::RenderTarget& t, const sf::FloatRect& view, float time, float wind) const;

private:
	struct Blade {
		V2 base;
		float h, lean, w;
		sf::Color col;
		bool reed;
	};
	struct Root {
		V2 a, c, b;
		float phase;
	};
	struct Chunk {
		std::unique_ptr<sf::RenderTexture> rt;
		sf::Vector2f pos;
	};
	void buildGeometry(const LevelData& L, int seed);

	const Assets* myAssets = nullptr;
	Palette pal;
	Biome biome = Biome::Canopy;
	Canvas fill, detail;
	std::vector<Blade> blades;  // sorted by x
	std::vector<Root> roots;    // sorted by a.x
	std::vector<V2> moss;       // glowing dots (hollow)
	std::vector<Chunk> chunks;
	float levelW = 0.f, levelH = 0.f;
};
