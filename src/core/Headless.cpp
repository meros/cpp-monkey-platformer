#include "Headless.h"

#include "Config.h"
#include "Level.h"
#include "World.h"

#include <cmath>
#include <cstdio>
#include <memory>

int runValidateLevels() {
	int failures = 0;
	int bananas = 0;
	for (int i = 1; i <= NUM_LEVELS; ++i) {
		LevelData L;
		std::string err;
		if (!loadLevelIndex(i, L, err)) {
			std::printf("level%02d FAIL  %s\n", i, err.c_str());
			++failures;
			continue;
		}
		std::vector<std::string> errs = validateLevel(L);
		// also build the physics world once: catches Box2D asserts / bad objects
		World w(L);
		bananas += w.bananasTotal;
		if (errs.empty()) {
			std::printf("level%02d OK    %-16s %-7s %3dx%-3d bananas=%d signs=%zu\n", i, L.name.c_str(),
			            biomeName(L.biome), L.width, L.height, w.bananasTotal, L.signs.size());
		} else {
			++failures;
			std::printf("level%02d FAIL  %s\n", i, L.name.c_str());
			for (const auto& e : errs) std::printf("    - %s\n", e.c_str());
		}
	}
	std::printf("total bananas: %d\n", bananas);
	if (failures) std::printf("%d level(s) failed validation\n", failures);
	return failures ? 1 : 0;
}

static bool finite(b2Vec2 v) { return std::isfinite(v.x) && std::isfinite(v.y); }

static int physicsTestLevel(int index) {
	LevelData L;
	std::string err;
	if (!loadLevelIndex(index, L, err)) {
		std::printf("level%02d: cannot load: %s\n", index, err.c_str());
		return 1;
	}
	World w(L);
	const float W = L.widthM(), H = L.heightM();
	int fails = 0;
	auto check = [&](int frame) {
		b2Vec2 p = w.player.pos();
		if (!finite(p) || !finite(w.player.vel())) {
			std::printf("level%02d: NaN player state at frame %d\n", index, frame);
			return false;
		}
		if (w.player.alive() && (p.x < -0.01f || p.x > W + 0.01f)) {
			std::printf("level%02d: player left the map horizontally at frame %d (x=%.2f)\n", index, frame, p.x);
			return false;
		}
		for (b2Body* b = w.b2->GetBodyList(); b; b = b->GetNext()) {
			if (!b->IsEnabled() || b->GetType() == b2_staticBody) continue;
			b2Vec2 q = b->GetPosition();
			if (!finite(q)) {
				std::printf("level%02d: NaN body at frame %d\n", index, frame);
				return false;
			}
			if (q.x < -2.f || q.x > W + 2.f || q.y > H + 4.f || q.y < -40.f) {
				std::printf("level%02d: body out of bounds (%.2f, %.2f) at frame %d\n", index, q.x, q.y, frame);
				return false;
			}
		}
		return true;
	};
	Input in;
	for (int f = 0; f < 600 && !fails; ++f) {
		in = Input();
		w.step(in);
		if (!check(f)) ++fails;
	}
	for (int f = 0; f < 600 && !fails; ++f) {
		in = Input();
		in.dir = 1;
		bool jump = (f / 20) % 2 == 0;
		in.jumpHeld = jump;
		in.jumpPressed = jump && (f % 20 == 0);
		w.step(in);
		if (!check(600 + f)) ++fails;
	}
	if (!fails && !(w.player.alive() || w.player.state == PState::Dead)) fails = 1;
	b2Vec2 p = w.player.pos();
	std::printf("level%02d %s  player=(%.2f, %.2f) deaths=%d bananas=%d\n", index, fails ? "FAIL" : "PASS", p.x,
	            p.y, w.deaths, w.bananas);
	return fails;
}

int runPhysicsTest(int level) {
	int fails = 0;
	if (level > 0) return physicsTestLevel(level) ? 1 : 0;
	for (int i = 1; i <= NUM_LEVELS; ++i) fails += physicsTestLevel(i);
	return fails ? 1 : 0;
}
