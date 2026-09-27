#include "Camera.h"

#include "core/Config.h"
#include "core/World.h"

#include <algorithm>
#include <cmath>

using namespace cfg;

V2 Camera::clampC(V2 c) const {
	float hw = WORLD_VIEW_W * 0.5f, hh = WORLD_VIEW_H * 0.5f;
	c.x = levelW <= WORLD_VIEW_W ? levelW * 0.5f : std::clamp(c.x, hw, levelW - hw);
	c.y = levelH <= WORLD_VIEW_H ? levelH * 0.5f : std::clamp(c.y, hh, levelH - hh);
	return c;
}

void Camera::reset(const World& w) {
	setLevelSize(w.level.width * TILE_PX, w.level.height * TILE_PX);
	b2Vec2 p = w.player.pos();
	lookAhead = w.player.facing * 2.f * TILE_PX;
	pos = clampC(V2(p.x * PPM + lookAhead, p.y * PPM - TILE_PX));
	shakeAmp = 0.f;
	shakeOffset = V2(0.f, 0.f);
}

void Camera::update(const World& w, float dt) {
	b2Vec2 p = w.player.pos();
	float targetLook = w.player.facing * 2.f * TILE_PX;
	float step = 2.f * 2.f * TILE_PX / 0.4f * dt;
	lookAhead += std::clamp(targetLook - lookAhead, -step, step);
	V2 target(p.x * PPM + lookAhead, p.y * PPM - TILE_PX);

	pos.x += (target.x - pos.x) * (1.f - std::exp(-8.f * dt));
	float dy = target.y - pos.y;
	const float dead = 1.5f * TILE_PX;
	bool groundedHere = w.player.grounded && w.player.state == PState::Ground;
	if (groundedHere) {
		pos.y += dy * (1.f - std::exp(-12.f * dt));
	} else if (std::fabs(dy) > dead) {
		float goal = target.y - (dy > 0 ? dead : -dead);
		pos.y += (goal - pos.y) * (1.f - std::exp(-6.f * dt));
	}
	// falling fast: never let Pip leave the view
	float maxOff = WORLD_VIEW_H * 0.5f - 60.f;
	pos.y = std::clamp(pos.y, p.y * PPM - maxOff, p.y * PPM + maxOff);
	pos = clampC(pos);

	if (shakeTime > 0.f) {
		shakeTime -= dt;
		float a = shakeAmp * std::max(0.f, shakeTime / shakeDur);
		shakeSeed = shakeSeed * 1664525u + 1013904223u;
		float rx = ((shakeSeed >> 8) & 0xFFFF) / 65535.f * 2.f - 1.f;
		shakeSeed = shakeSeed * 1664525u + 1013904223u;
		float ry = ((shakeSeed >> 8) & 0xFFFF) / 65535.f * 2.f - 1.f;
		shakeOffset = V2(rx * a, ry * a);
	} else {
		shakeOffset = V2(0.f, 0.f);
	}
}

void Camera::shake(float amp, float dur) {
	if (amp >= shakeAmp * std::max(0.f, shakeTime / std::max(0.001f, shakeDur))) {
		shakeAmp = amp;
		shakeDur = dur;
		shakeTime = dur;
	}
}

sf::View Camera::view() const {
	V2 c = center();
	// round to whole pixels so the sprite and tiles stay crisp
	c.x = std::round(c.x);
	c.y = std::round(c.y);
	return sf::View(c, V2(WORLD_VIEW_W, WORLD_VIEW_H));
}

sf::FloatRect Camera::rect() const {
	V2 c = center();
	return sf::FloatRect(c.x - WORLD_VIEW_W * 0.5f, c.y - WORLD_VIEW_H * 0.5f, WORLD_VIEW_W, WORLD_VIEW_H);
}
