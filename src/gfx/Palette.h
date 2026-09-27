// Leafwind -- biome palettes and UI colours (docs/DESIGN.md 5.1).
#pragma once

#include "core/Level.h"

#include <SFML/Graphics/Color.hpp>

struct Palette {
	sf::Color skyTop, skyBot, far, mid, near, earth, earthDark, grass, grassLight, accent, water, fog;
};

const Palette& paletteFor(Biome b);

namespace ui {
const sf::Color Cream(0xFF, 0xF4, 0xD6);
const sf::Color Ink(0x2E, 0x24, 0x18);
const sf::Color Banana(0xFF, 0xD2, 0x3F);
const sf::Color Danger(0xE0, 0x46, 0x3C);
const sf::Color GoldCore(0xFF, 0xD9, 0x72);
const sf::Color GoldEdge(0xFF, 0xB1, 0x3B);
} // namespace ui

sf::Color lerp(sf::Color a, sf::Color b, float t);
sf::Color withAlpha(sf::Color c, int a);
sf::Color scale(sf::Color c, float f); // multiply rgb
sf::Color hex(unsigned v);
