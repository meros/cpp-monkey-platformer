// Leafwind -- follow camera with look-ahead, dead zone and shake (docs/DESIGN.md 2.6).
#pragma once

#include "Draw.h"

class World;

class Camera {
public:
	void reset(const World& w);            // snap onto the player
	void update(const World& w, float dt);
	void shake(float amplitudePx, float duration);
	void setLevelSize(float wpx, float hpx) {
		levelW = wpx;
		levelH = hpx;
	}
	V2 center() const { return pos + shakeOffset; }
	sf::View view() const;
	sf::FloatRect rect() const;
	void setCenter(V2 c) { pos = clampC(c); }

	V2 pos{400.f, 300.f};
	float lookAhead = 0.f;

private:
	V2 clampC(V2 c) const;
	float levelW = 800.f, levelH = 600.f;
	float shakeAmp = 0.f, shakeTime = 0.f, shakeDur = 0.f;
	V2 shakeOffset{0.f, 0.f};
	unsigned shakeSeed = 1;
};
