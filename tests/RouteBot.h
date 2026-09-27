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
	int bounces = 0, grabs = 0;
	void step(const Input& in) {
		w->step(in);
		++frames;
		for (const GameEvent& e : w->events) {
			if (e.type == Ev::Bounce) ++bounces;
			if (e.type == Ev::RopeGrab) ++grabs;
		}
		w->events.clear();
		if (trace > 0 && frames % trace == 0)
			std::printf("        f=%5d tile (%6.2f, feet %6.2f) st=%d v=(%5.2f,%6.2f) in=%2d%s%s seg=%d\n", frames, tx(), feet(),
			            static_cast<int>(p().state), p().vel().x, p().vel().y, in.dir, in.jumpHeld ? " J" : "",
			            in.jumpPressed ? "!" : "", p().ropeSeg);
		if (trace > 0 && frames % trace == 0 && std::getenv("ROUTE_TRACE_OBJ")) {
			for (auto& l : w->fallLogs)
				if (l->state != FallLog::Hanging)
					std::printf("            log x=%.2f y=%.2f vy=%.2f st=%d\n", l->body->GetPosition().x / cfg::TILE,
					            l->body->GetPosition().y / cfg::TILE, l->body->GetLinearVelocity().y, static_cast<int>(l->state));
			if (std::getenv("ROUTE_TRACE_ROPE"))
				for (auto& r : w->ropes) {
					b2Vec2 a = r->anchor;
					if (std::fabs(a.x / cfg::TILE - tx()) > 6.f) continue;
					b2Vec2 l = r->segs.back()->GetPosition();
					std::printf("            rope anchor x=%.2f bottom seg (%.2f, %.2f) ignored=%d\n", a.x / cfg::TILE, l.x / cfg::TILE,
					            l.y / cfg::TILE, p().ignoredRope == r.get());
				}
			for (auto& bo : w->boulders)
				if (bo->alive)
					std::printf("            boulder x=%.2f y=%.2f v=(%.2f,%.2f)\n", bo->body->GetPosition().x / cfg::TILE,
					            bo->body->GetPosition().y / cfg::TILE, bo->body->GetLinearVelocity().x, bo->body->GetLinearVelocity().y);
		}
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
		// hold position (wind can drift an idle Pip)
		return until("settle", 240,
		             [&] {
			             float d = x - tx();
			             return hold(d > 0.12f ? 1 : (d < -0.12f ? -1 : 0));
		             },
		             [&] { return std::fabs(p().vel().x) < 0.3f && std::fabs(x - tx()) < 0.2f && grounded(); });
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
	// Walk toward x (right if dir=1, left if -1); hop over (or onto) beetles that come near.
	bool passBeetles(float x, int maxF = 1200, int dir = 1) {
		int air = 0;
		return until("pass beetles", maxF,
		             [&] {
			             Input in = hold(dir);
			             if (grounded()) {
				             air = 0;
				             for (auto& be : w->beetles) {
					             if (be->dead || !be->body) continue;
					             b2Vec2 bp = be->body->GetPosition();
					             float dx = (bp.x / cfg::TILE - tx()) * dir, dy = bp.y / cfg::TILE - (feet() - 0.3f);
					             if (dx > 0.6f && dx < 3.2f && std::fabs(dy) < 0.8f) {
						             in.jumpPressed = true;
						             in.jumpHeld = true;
					             }
				             }
			             } else {
				             in.jumpHeld = ++air < 20;
			             }
			             return in;
		             },
		             [&] { return grounded() && (tx() - x) * dir >= 0.f; });
	}
	// From the ground (or a ledge) next to a mushroom at tile column mushX: hop onto its cap,
	// holding Space for the big bounce if big, then steer toward landX until landed().
	bool bounce(float mushX, bool big, float landX, const std::function<bool()>& landed, int maxF = 900) {
		int f = 0;
		bool bounced = false;
		int b0 = bounces;
		return until("bounce", maxF,
		             [&] {
			             Input in;
			             float target = bounced ? landX : mushX + 0.5f;
			             float diff = target - tx();
			             in.dir = diff > 0.15f ? 1 : (diff < -0.15f ? -1 : 0);
			             if (f == 0) in.jumpPressed = true;
			             in.jumpHeld = f < 4 || (big && !bounced && p().vel().y > 0.f) || (bounced && f < 400);
			             if (bounces > b0) bounced = true;
			             ++f;
			             return in;
		             },
		             [&] { return bounced && landed(); });
	}
	// Wait (standing) until the target body is within [minD, maxD] tiles horizontally (signed in
	// dir) and not more than maxRise tiles above the feet, then jump onto it, steering to its centre.
	bool hopOnto(b2Body* target, int dir, float minD, float maxD, float maxRise = 2.5f, int maxF = 1500) {
		bool jumped = false;
		int f = 0;
		return until("hop onto platform", maxF,
		             [&] {
			             Input in;
			             b2Vec2 tp = target->GetPosition();
			             float d = (tp.x / cfg::TILE - tx()) * dir;
			             float rise = feet() - tp.y / cfg::TILE;
			             if (!jumped) {
				             if (grounded() && d >= minD && d <= maxD && rise <= maxRise && rise > -4.f) {
					             jumped = true;
					             in.jumpPressed = true;
					             in.jumpHeld = true;
					             in.dir = dir;
				             }
			             } else {
				             ++f;
				             float diff = tp.x / cfg::TILE - tx();
				             in.dir = diff > 0.25f ? 1 : (diff < -0.25f ? -1 : 0);
				             in.jumpHeld = f < 20;
			             }
			             return in;
		             },
		             [&] { return jumped && grounded() && p().groundBody == target; });
	}
	// Ride the platform we stand on (keeping to its centre) until cond().
	bool ride(const std::function<bool()>& cond, int maxF = 1500) {
		b2Body* body = p().groundBody;
		return until("ride", maxF,
		             [&] {
			             Input in;
			             if (body) {
				             float diff = body->GetPosition().x / cfg::TILE - tx();
				             in.dir = diff > 0.3f ? 1 : (diff < -0.3f ? -1 : 0);
			             }
			             return in;
		             },
		             cond);
	}
	// Advance right through hanging logs and beetles: poke each hanging log's trigger zone and
	// back off until it has landed, jump walls / landed logs, hop beetles when nothing is falling.
	bool gauntlet(float x, int maxF = 3000) {
		FallLog* waitFor = nullptr;
		int retreat = 0, air = 0;
		return until("gauntlet", maxF,
		             [&] {
			             Input in = hold(1);
			             if (!grounded()) {
				             in.jumpHeld = ++air < 20;
				             return in;
			             }
			             air = 0;
			             if (waitFor) {
				             if (waitFor->state == FallLog::Hanging) return in; // keep stepping in until it creaks
				             in.dir = retreat-- > 0 ? -1 : 0;
				             if (waitFor->state == FallLog::Resting && retreat <= 0) waitFor = nullptr;
				             return in;
			             }
			             bool danger = false;
			             for (auto& lg : w->fallLogs) {
				             float l = lg->x0 * 1.f, r = l + lg->tiles;
				             if (lg->state == FallLog::Hanging && l - (tx() + 0.4f) < 0.6f && l - (tx() + 0.4f) > -0.2f &&
				                 feet() > lg->y0 && feet() < lg->y0 + 8) {
					             waitFor = lg.get();
					             retreat = 40;
					             in.dir = 1; // step into the zone this frame
					             return in;
				             }
				             if ((lg->state == FallLog::Triggered || lg->state == FallLog::Falling) && tx() > l - 3.f && tx() < r + 3.f)
					             danger = true;
			             }
			             int tx0 = static_cast<int>(std::floor(tx() + 0.45f)), ty0 = static_cast<int>(std::floor(feet() - 0.5f));
			             bool wall = L.solid(tx0, ty0);
			             for (auto& lg : w->fallLogs) {
				             if (lg->state != FallLog::Resting) continue;
				             b2Vec2 lp = lg->body->GetPosition();
				             float l = lp.x / cfg::TILE - lg->tiles * 0.5f;
				             if (l - tx() > 0.3f && l - tx() < 0.9f && std::fabs(lp.y / cfg::TILE - (feet() - 0.5f)) < 0.8f) wall = true;
			             }
			             bool beetle = false;
			             for (auto& be : w->beetles) {
				             if (be->dead || !be->body) continue;
				             b2Vec2 bp = be->body->GetPosition();
				             float dx = bp.x / cfg::TILE - tx(), dy = bp.y / cfg::TILE - (feet() - 0.3f);
				             if (dx > 0.6f && dx < 3.2f && std::fabs(dy) < 0.8f) beetle = true;
			             }
			             if ((wall || beetle) && !danger) {
				             in.jumpPressed = true;
				             in.jumpHeld = true;
			             } else if (danger && beetle) {
				             in.dir = -1;
			             }
			             return in;
		             },
		             [&] { return grounded() && tx() >= x; });
	}
	Entity* groundEntity() { return grounded() && p().groundBody ? entityOf(p().groundBody) : nullptr; }
	bool onKind(Kind k) {
		Entity* e = groundEntity();
		return e && e->kind == k;
	}
	void report(const char* name) const {
		if (ok)
			std::printf("    %s: completed in %.1f s of game time, deaths %d, bananas %d/%d, fig %d\n", name, frames / 60.f,
			            w->deaths, w->bananas, w->bananasTotal, w->fig);
		else std::printf("    %s: FAILED: %s\n", name, failure.c_str());
	}
};
