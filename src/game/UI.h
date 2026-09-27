// Leafwind -- UI drawing helpers: text, panels, icons, story-card art (docs/DESIGN.md 5.9).
#pragma once

#include "gfx/Assets.h"
#include "gfx/Draw.h"
#include "gfx/Palette.h"

#include <string>
#include <vector>

namespace uidraw {

enum Align { Left, Center, Right };

sf::String utf8(const std::string& s);
float textWidth(const sf::Font& f, const std::string& s, unsigned size);
std::vector<std::string> wrap(const sf::Font& f, const std::string& s, unsigned size, float maxWidth);

void text(sf::RenderTarget& t, const sf::Font& f, const std::string& s, unsigned size, V2 pos, sf::Color col,
          Align align = Left, float alpha = 1.f);
void textShadow(sf::RenderTarget& t, const sf::Font& f, const std::string& s, unsigned size, V2 pos, sf::Color col,
                sf::Color shadow, V2 offset, Align align = Left, float alpha = 1.f);

void bananaIcon(Canvas& c, V2 p, float s, float alpha = 1.f);
void figIcon(Canvas& c, V2 p, float s, bool gold, float alpha = 1.f);
void skullLeafIcon(Canvas& c, V2 p, float s, float alpha = 1.f);
void vineKnot(Canvas& c, V2 p, float s, sf::Color col);
void swirl(Canvas& c, V2 p, float s, sf::Color col);
void bigLeaf(Canvas& c, V2 p, float w, float h, float ang, sf::Color body, sf::Color vein);

std::string formatTime(float seconds);

} // namespace uidraw
