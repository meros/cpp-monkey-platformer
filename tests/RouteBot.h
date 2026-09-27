// Leafwind -- a scripted "route bot" for headless playthrough tests. Routes are sequences of
// macro-actions (run to x, jump at x, grab/swing/release vines, ride things...) executed on the
// real simulation with real inputs, frame by frame.
#pragma once

#include "core/Config.h"
#include "core/Level.h"
#include "core/World.h"

#include <cmath>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>

class RouteBot {
public:
	explicit RouteBot(int level, bool verbose = false) : verbose(verbose) {
		std::string err;
		if (!loadLevelIndex(level, L, err)) {
			std::printf("cannot load level %d: %s\n", level, err.c_str());
			std::exit(2);
		}
		if (const char* t = std::getenv("ROUTE_TRACE")) trace = std::atoi(t);
		w = std::make_unique<World>(L);
		idle(20);
	}

	LevelData L;
	std::unique_ptr<World> w;
	bool ok = true;
	bool verbose = false;
	std::string failure;
	int frames = 0;
	int deathsAllowed = 0;

	Player& p() { return w->player; }
	float tx() const { return w->player.pos().x / cfg::TILE; }                // centre, tiles
	float feet() const { return w->player.bottom() / cfg::TILE; }             // feet, tiles
	bool grounded() { return p().grounded && p().state == PState::Ground; }

	int trace = 0; // print every N frames when > 0
	void step(const Input& in) {
		w->step(in);
		++frames;
		if (trace > 0 && frames % trace == 0)
			std::printf("        f=%5d tile (%6.2f, feet %6.2f) st=%d v=(%5.2f,%6.2f) in=%2d%s%s seg=%d\n", frames, tx(), feet(),
			            static_cast<int>(p().state), p().vel().x, p().vel().y, in.dir, in.jumpHeld ? " J" : "",
			            in.jumpPressed ? "!" : "", p().ropeSeg);
		if (w->deaths > deathsAllowed && ok) fail("died");
	}
	void idle(int n) {
		for (int i = 0; i < n && ok; ++i) step(Input());
	}
	static Input hold(int dir, bool jump = false, bool up = false, bool down = false) {
		Input in;
		in.dir = dir;
		in.jumpHeld = jump;
		in.up = up;
		in.down = down;
		return in;
	}

	void fail(const std::string& why) {
		if (!ok) return;
		ok = false;
		char buf[256];
		std::snprintf(buf, sizeof(buf), "%s at frame %d, Pip at tile (%.2f, feet %.2f) state %d vel (%.2f, %.2f)",
		              why.c_str(), frames, tx(), feet(), static_cast<int>(p().state), p().vel().x, p().vel().y);
		failure = buf;
	}

	// Generic: apply ctl each frame until done() or maxFrames.
	bool until(const char* what, int maxFrames, const std::function<Input()>& ctl, const std::function<bool()>& done) {
		if (!ok) return false;
		for (int f = 0; f < maxFrames && ok; ++f) {
			step(ctl());
			if (done()) {
				if (verbose)
					std::printf("      ok: %-28s f=%5d tile (%.2f, feet %.2f)\n", what, frames, tx(), feet());
				return true;
			}
		}
		fail(std::string("timeout: ") + what);
		return false;
	}

	// Walk (on the ground) until the centre crosses x, then stop.
	bool walkTo(float x, int maxF = 900) {
		int dir = x > tx() ? 1 : -1;
		bool r = until("walkTo", maxF, [&] { return hold(dir); }, [&] { return (tx() - x) * dir >= 0.f; });
		return r;
	}
	// Walk to x and come to rest there.
	bool stopAt(float x) {
		if (!walkTo(x)) return false;
		return until("settle", 120, [&] { return hold(0); }, [&] { return std::fabs(p().vel().x) < 0.05f && grounded(); });
	}
	// Run toward dir, press jump when the centre crosses x (or immediately if x is NaN), hold jump
	// for holdF frames, keep steering airDir until the landing condition.
	bool jumpAt(float x, int dir, int holdF, int airDir, const std::function<bool()>& landed, int maxF = 600) {
		if (!std::isnan(x) && !until("run-up", maxF, [&] { return hold(dir); }, [&] { return (tx() - x) * dir >= 0.f; }))
			return false;
		int f = 0;
		return until("jump+air", maxF,
		             [&] {
			             Input in = hold(f < holdF ? dir : airDir, f < holdF);
			             in.jumpPressed = f == 0;
			             ++f;
			             return in;
		             },
		             [&] { return f > 3 && landed(); });
	}
	bool onGroundRightOf(float x) { return grounded() && tx() > x; }
	bool onGroundLeftOf(float x) { return grounded() && tx() < x; }
	bool onRope() { return p().state == PState::Rope; }

	// On a vine: climb to a segment index.
	bool climbTo(int seg) {
		return until("climb", 600,
		             [&] { return hold(0, false, p().ropeSeg > seg, p().ropeSeg < seg); },
		             [&] { return onRope() && p().ropeSeg == seg; });
	}
	// Pump the swing and let go (jump) on the forward swing once Pip is past minAngle (radians,
	// measured from straight down at the anchor) and still moving in dir; then steer until landed().
	bool swingRelease(int dir, float minAngle, const std::function<bool()>& landed, bool hopHold = true, int maxF = 1200) {
		if (!onRope()) {
			fail("swingRelease: not on a vine");
			return false;
		}
		bool released = false;
		int after = 0, onFor = 0;
		return until("swing+release", maxF,
		             [&] {
			             Input in;
			             if (!released) {
				             ++onFor;
				             float vx = p().vel().x;
				             in.dir = vx >= 0.f ? 1 : -1; // pump with the swing
				             b2Vec2 d = p().pos() - (p().rope ? p().rope->anchor : p().pos());
				             float ang = std::atan2(d.x, d.y);
				             if (onFor > 15 && ang * dir > minAngle && vx * dir > 0.3f) {
					             in.jumpPressed = true;
					             in.jumpHeld = hopHold;
					             in.dir = dir;
					             released = true;
				             }
			             } else {
				             in.dir = dir;
				             in.jumpHeld = hopHold && after < 20;
				             ++after;
			             }
			             return in;
		             },
		             [&] { return released && after > 2 && landed(); });
	}
	// Cross water toward +x by hopping along floating logs (or swimming and hopping out), aiming
	// each jump at the next platform like a player would. Done when standing on static ground
	// right of targetX.
	struct Plat {
		float l, r, top; // tiles
		b2Body* body;
	};
	std::vector<Plat> platforms(float targetX) {
		std::vector<Plat> v;
		for (auto& lg : w->floatLogs) {
			if (!lg->alive) continue;
			b2AABB bb;
			bb.lowerBound.Set(1e9f, 1e9f);
			bb.upperBound.Set(-1e9f, -1e9f);
			for (b2Fixture* f = lg->body->GetFixtureList(); f; f = f->GetNext()) bb.Combine(f->GetAABB(0));
			v.push_back({bb.lowerBound.x / cfg::TILE, bb.upperBound.x / cfg::TILE, bb.lowerBound.y / cfg::TILE, lg->body});
		}
		v.push_back({targetX, targetX + 6.f, 0.f, nullptr});
		return v;
	}
	bool crossRight(float targetX, int maxF = 3000) {
		int air = 0;
		float aim = targetX + 1.f;
		return until("cross water", maxF,
		             [&] {
			             Input in;
			             PState st = p().state;
			             auto plats = platforms(targetX);
			             // current support's right edge
			             float curR = tx();
			             if (st == PState::Ground && p().groundBody) {
				             for (auto& pl : plats)
					             if (pl.body == p().groundBody) curR = pl.r;
				             if (p().groundBody->GetType() == b2_staticBody) {
					             int x = static_cast<int>(std::floor(tx()));
					             int y = static_cast<int>(std::floor(feet() + 0.1f));
					             curR = static_cast<float>(x);
					             while (L.solid(curR, y)) curR += 1.f;
				             }
			             }
			             // next platform: nearest whose left edge is beyond our support's right edge
			             const Plat* next = nullptr;
			             for (auto& pl : plats)
				             if (pl.l > curR - 0.3f && (!next || pl.l < next->l)) next = &pl;
			             if (st == PState::Swim) {
				             in.dir = 1;
				             in.jumpHeld = true;
				             bool near = next && next->l - tx() < 1.3f;
				             bool bankAhead = L.solid(static_cast<int>(tx() + 0.6f), static_cast<int>(feet() - 0.8f));
				             in.jumpPressed = near || bankAhead;
				             if (next) aim = (next->l + next->r) * 0.5f;
			             } else if (st == PState::Ground) {
				             air = 0;
				             float cx = next ? std::min((next->l + next->r) * 0.5f, next->l + 1.5f) : tx() + 1.f;
				             float d = cx - tx();
				             if (d < 1.2f) in.dir = 1;                  // just walk over
				             else if (d < 5.2f) {                       // in range: jump at it
					             in.dir = 1;
					             in.jumpPressed = true;
					             in.jumpHeld = true;
					             aim = cx;
				             } else if (p().groundBody && p().groundBody->GetType() == b2_staticBody) {
					             in.dir = 1;                            // from a bank: just swim for it
				             } else {                                   // on a log: wait near its edge
					             float edge = curR - 0.7f;
					             in.dir = tx() < edge - 0.2f ? 1 : (tx() > edge + 0.2f ? -1 : 0);
				             }
			             } else {
				             ++air;
				             float diff = aim - tx();
				             in.dir = diff > 0.3f ? 1 : (diff < -0.3f ? -1 : 0);
				             in.jumpHeld = air < 20;
			             }
			             return in;
		             },
		             [&] {
			             return grounded() && tx() > targetX && p().groundBody && p().groundBody->GetType() == b2_staticBody;
		             });
	}
	void report(const char* name) const {
		if (ok)
			std::printf("    %s: completed in %.1f s of game time, deaths %d, bananas %d/%d, fig %d\n", name, frames / 60.f,
			            w->deaths, w->bananas, w->bananasTotal, w->fig);
		else std::printf("    %s: FAILED: %s\n", name, failure.c_str());
	}
};
