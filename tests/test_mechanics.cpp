// Leafwind -- headless scripted-input tests of the core mechanics (docs/DESIGN.md 2-3, 8).
// Each test loads a real level, places Pip, feeds inputs frame by frame and checks behaviour.
#include "core/Config.h"
#include "core/Level.h"
#include "core/World.h"

#include <cmath>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace {

int gFails = 0;

#define CHECK(cond, ...)                                                                                     \
	do {                                                                                                     \
		if (!(cond)) {                                                                                       \
			std::printf("    FAIL: " __VA_ARGS__);                                                         \
			std::printf("   [%s:%d]\n", __FILE__, __LINE__);                                                \
			++gFails;                                                                                        \
		}                                                                                                    \
	} while (0)

struct Sim {
	LevelData level;
	std::unique_ptr<World> w;
	explicit Sim(int idx) {
		std::string err;
		if (!loadLevelIndex(idx, level, err)) {
			std::printf("cannot load level %d: %s\n", idx, err.c_str());
			std::exit(2);
		}
		w = std::make_unique<World>(level);
	}
	Player& p() { return w->player; }
	b2Vec2 pos() { return w->player.pos(); }
	void step(int dir = 0, bool jump = false, bool jumpPress = false, bool up = false, bool down = false) {
		Input in;
		in.dir = dir;
		in.jumpHeld = jump;
		in.jumpPressed = jumpPress;
		in.up = up;
		in.down = down;
		w->step(in);
	}
	void idle(int n) {
		for (int i = 0; i < n; ++i) step();
	}
	// Place Pip standing with his feet at the bottom of tile (tx, ty).
	void placeOnTile(int tx, int ty) { w->placePlayer(tx * 0.5f + 0.25f, (ty + 1) * 0.5f - 0.3f); }
};

// Hold `dir` and press jump whenever the scripted predicate says so, until done() or timeout.
bool runUntil(Sim& s, int maxFrames, const std::function<Input(Sim&, int)>& ctl,
              const std::function<bool(Sim&)>& done) {
	for (int f = 0; f < maxFrames; ++f) {
		Input in = ctl(s, f);
		s.w->step(in);
		if (done(s)) return true;
	}
	return false;
}

void testJumpFeel() {
	std::printf("[feel] run speed, jump apex, tap jump, jump length\n");
	Sim s(1);
	s.idle(30);
	CHECK(s.p().state == PState::Ground, "Pip should be grounded at start (state %d)\n", (int)s.p().state);
	float y0 = s.pos().y;
	s.step(0, true, true);
	float minY = y0;
	for (int i = 0; i < 60; ++i) {
		s.step(0, true);
		minY = std::fmin(minY, s.pos().y);
	}
	float apexTiles = (y0 - minY) / 0.5f;
	std::printf("    full jump apex: %.2f tiles (design 3.24)\n", apexTiles);
	CHECK(std::fabs(apexTiles - 3.24f) < 0.25f, "apex %.2f\n", apexTiles);
	s.idle(40);
	y0 = s.pos().y;
	s.step(0, true, true);
	minY = y0;
	for (int i = 0; i < 60; ++i) {
		s.step(0, false);
		minY = std::fmin(minY, s.pos().y);
	}
	float tapTiles = (y0 - minY) / 0.5f;
	std::printf("    tapped jump apex: %.2f tiles (design 1.35)\n", tapTiles);
	CHECK(std::fabs(tapTiles - 1.35f) < 0.25f, "tap %.2f\n", tapTiles);

	// run speed on the flat start area
	s.placeOnTile(3, 20);
	s.idle(20);
	for (int i = 0; i < 20; ++i) s.step(1);
	float vx = s.p().vel().x;
	std::printf("    run speed after 0.33 s: %.2f m/s (design 4.5)\n", vx);
	CHECK(std::fabs(vx - 4.5f) < 0.05f, "run speed %.2f\n", vx);
	// flat jump length at full run: take off at x0, land back on the same floor
	float x0 = s.pos().x;
	float yGround = s.pos().y;
	s.step(1, true, true);
	int f = 0;
	while (f < 120) {
		s.step(1, true);
		++f;
		if (s.p().grounded && s.pos().y >= yGround - 0.01f) break;
	}
	float len = (s.pos().x - x0) / 0.5f;
	std::printf("    flat jump length at full run: %.2f tiles (design ~6)\n", len);
	CHECK(len > 5.5f && len < 6.8f, "jump length %.2f\n", len);
}

void testCoyoteAndBuffer() {
	std::printf("[feel] coyote time and jump buffer\n");
	Sim s(1);
	// run off the ledge at the 3-wide pit (x=34..36 air, ground row 21) and jump late
	s.placeOnTile(28, 20);
	s.idle(10);
	bool jumped = false;
	float startY = 0.f;
	for (int i = 0; i < 90 && !jumped; ++i) {
		bool overPit = s.pos().x > 34 * 0.5f + 0.2f;
		if (overPit && !s.p().grounded && s.p().coyote > 0.f) {
			startY = s.pos().y;
			s.step(1, true, true);
			jumped = s.p().vel().y < -5.f;
			break;
		}
		s.step(1);
	}
	CHECK(jumped, "coyote jump did not fire\n");
	(void)startY;
	// buffer: press jump shortly before landing
	s.placeOnTile(10, 17);
	int pressAt = -1;
	for (int i = 0; i < 120; ++i) {
		bool press = false;
		if (pressAt < 0 && !s.p().grounded && s.p().vel().y > 0.f && s.p().bottom() > 10.5f - 0.3f) {
			// floor (row 21) top at 10.5 m: press ~2-3 frames before touching down
			press = true;
			pressAt = i;
		}
		s.step(0, press || (pressAt >= 0 && i - pressAt < 10), press);
		if (pressAt >= 0 && s.p().vel().y < -5.f) break;
	}
	CHECK(pressAt >= 0 && s.p().vel().y < -5.f, "buffered jump did not fire on landing\n");
}

void testOneWay() {
	std::printf("[L1] one-way branch: jump through from below, stand on it, drop through\n");
	Sim s(1);
	// branch '=====' at row 18, x=40..44; ground row 21 (feet at row 20)
	s.placeOnTile(42, 20);
	s.idle(10);
	s.step(0, true, true);
	for (int i = 0; i < 70; ++i) s.step(0, true);
	s.idle(30);
	float feet = s.p().bottom();
	std::printf("    feet after jump: %.2f m (branch top 9.0)\n", feet);
	CHECK(std::fabs(feet - 9.0f) < 0.05f && s.p().grounded, "not standing on the branch\n");
	for (int i = 0; i < 30; ++i) s.step(0, false, false, false, true);
	s.idle(40);
	CHECK(s.p().bottom() > 10.4f, "drop-through failed (feet %.2f)\n", s.p().bottom());
}

// Simple reactive bot: run right; jump when a wall or a gap is ahead.
Input botRight(Sim& s, int) {
	Input in;
	in.dir = 1;
	const LevelData& L = s.level;
	b2Vec2 p = s.pos();
	int tx = (int)std::floor(p.x / 0.5f);
	int ty = (int)std::floor((s.p().bottom() - 0.01f) / 0.5f);
	bool wallAhead = L.solid(tx + 1, ty) || L.solid(tx + 1, ty - 1);
	bool gapAhead = !L.solid(tx + 1, ty + 1) && !L.solid(tx + 2, ty + 1) && L.at(tx + 1, ty + 1) != '=';
	bool jump = s.p().grounded && (wallAhead || gapAhead);
	in.jumpPressed = jump;
	in.jumpHeld = jump || (!s.p().grounded && s.p().vel().y < 0.f);
	return in;
}

void testLevel1Complete() {
	std::printf("[L1] a simple run-right bot reaches the exit\n");
	Sim s(1);
	bool ok = runUntil(s, 60 * 60, botRight, [](Sim& s) { return s.w->complete; });
	std::printf("    complete=%d time=%.1fs deaths=%d bananas=%d/%d checkpoint=%d\n", ok, s.w->levelTimer,
	            s.w->deaths, s.w->bananas, s.w->bananasTotal, s.w->activeCheckpoint);
	CHECK(ok, "bot did not finish level 1\n");
	CHECK(s.w->activeCheckpoint >= 0, "checkpoint not activated\n");
}

void testRope() {
	std::printf("[L2] vine grab, climb, swing, release\n");
	Sim s(2);
	// first vine: R at (18,18), '|' down to row 24 -> length 7 tiles
	s.placeOnTile(14, 26);
	s.idle(10);
	bool grabbed = runUntil(
	    s, 120,
	    [](Sim& s, int f) {
		    Input in;
		    in.dir = 1;
		    in.jumpPressed = f == 5;
		    in.jumpHeld = f >= 5 && f < 30;
		    return in;
	    },
	    [](Sim& s) { return s.p().state == PState::Rope; });
	CHECK(grabbed, "did not grab the vine\n");
	if (!grabbed) return;
	int seg0 = s.p().ropeSeg;
	for (int i = 0; i < 30; ++i) s.step(0, false, false, true);
	std::printf("    grabbed segment %d, after climbing up 0.5 s: %d\n", seg0, s.p().ropeSeg);
	CHECK(s.p().ropeSeg < seg0 - 2, "climb up failed\n");
	// swing: pump right/left in rhythm with the swing
	float maxVx = 0.f;
	for (int i = 0; i < 240; ++i) {
		int dir = s.p().vel().x >= 0 ? 1 : -1;
		s.step(dir);
		maxVx = std::fmax(maxVx, std::fabs(s.p().vel().x));
	}
	std::printf("    max swing speed after pumping 4 s: %.2f m/s\n", maxVx);
	CHECK(maxVx > 3.f, "swing too weak\n");
	// stretch check: player stays within the vine length of the anchor
	b2Vec2 d = s.pos() - s.p().rope->anchor;
	CHECK(d.Length() < s.p().rope->lengthTiles * 0.5f + 0.2f, "rope stretched: %.2f\n", d.Length());
	s.step(0, true, true);
	CHECK(s.p().state == PState::Air, "release failed\n");
	s.idle(5);
	CHECK(s.p().state != PState::Rope, "instant re-grab after release\n");
}

void testLevel2FirstPit() {
	std::printf("[L2] cross the first pit with the first vine\n");
	Sim s(2);
	s.placeOnTile(12, 26);
	s.idle(10);
	int phase = 0;
	bool ok = runUntil(
	    s, 60 * 12,
	    [&](Sim& s, int f) {
		    Input in;
		    if (phase == 0) {
			    in.dir = 1;
			    if (s.pos().x > 7.4f && s.p().grounded) {
				    in.jumpPressed = true;
				    phase = 1;
			    }
			    in.jumpHeld = true;
		    } else if (phase == 1) {
			    in.dir = 1;
			    in.jumpHeld = true;
			    if (s.p().state == PState::Rope) phase = 2;
		    } else if (phase == 2) {
			    // pump, release when swinging right fast and rising
			    in.dir = s.p().vel().x >= 0 ? 1 : -1;
			    if (s.p().vel().x > 3.0f && s.p().vel().y < 0.f) {
				    in.jumpPressed = true;
				    in.jumpHeld = true;
				    phase = 3;
			    }
		    } else {
			    in.dir = 1;
			    in.jumpHeld = true;
		    }
		    (void)f;
		    return in;
	    },
	    [](Sim& s) { return s.p().grounded && s.pos().x > 21 * 0.5f && s.pos().y < 13.5f; });
	std::printf("    crossed=%d at x=%.2f deaths=%d\n", ok, s.pos().x, s.w->deaths);
	CHECK(ok, "could not cross the first vine pit\n");
}

void testBridge() {
	std::printf("[L3] rope bridge sag under Pip\n");
	Sim s(3);
	s.idle(120);
	// 14-plank bridge at row 16, x=91..106
	Bridge* br = nullptr;
	for (auto& b : s.w->bridges)
		if (b->planks.size() == 16) br = b.get();
	CHECK(br != nullptr, "16-plank bridge not found\n");
	if (!br) return;
	float restMid = br->planks[8]->GetPosition().y;
	s.w->placePlayer(br->planks[8]->GetPosition().x, restMid - 0.5f);
	s.idle(120);
	float loaded = br->planks[8]->GetPosition().y;
	float sagTiles = (loaded - br->leftAnchor.y) / 0.5f;
	std::printf("    16-plank bridge: rest sag %.2f tiles, loaded %.2f tiles (design ~1.6)\n",
	            (restMid - br->leftAnchor.y) / 0.5f, sagTiles);
	CHECK(sagTiles > 0.8f && sagTiles < 2.5f, "sag %.2f\n", sagTiles);
	CHECK(s.p().grounded && s.p().alive(), "Pip should stand on the bridge\n");
	// walk across to the right
	bool ok = runUntil(s, 60 * 6, [](Sim&, int) { Input in; in.dir = 1; return in; },
	                   [&](Sim& s) { return s.pos().x > br->rightAnchor.x + 0.5f; });
	CHECK(ok && s.p().alive(), "could not walk off the bridge (x=%.2f)\n", s.pos().x);
}

void testSwim() {
	std::printf("[L4] swimming, current, hop out\n");
	Sim s(4);
	// drop into river 1 (x 15..40, surface row 22)
	s.w->placePlayer(10.f, 10.5f);
	s.idle(90);
	CHECK(s.p().state == PState::Swim, "not swimming (state %d)\n", (int)s.p().state);
	float depth = s.pos().y - 11.f;
	std::printf("    rest depth: centre %.2f m below surface (design 0.25)\n", depth);
	CHECK(std::fabs(depth - 0.25f) < 0.12f, "rest depth %.2f\n", depth);
	float x0 = s.pos().x;
	s.idle(60);
	std::printf("    drift in the current over 1 s: %.2f m\n", s.pos().x - x0);
	CHECK(s.pos().x - x0 > 0.3f, "current does not push\n");
	// dive
	for (int i = 0; i < 40; ++i) s.step(0, false, false, false, true);
	CHECK(s.pos().y > 12.f, "dive failed\n");
	s.idle(60);
	s.step(0, true, true);
	CHECK(s.p().vel().y < -5.f, "hop out of water failed\n");
}

void testLogs() {
	std::printf("[L4] floating logs float and carry Pip\n");
	Sim s(4);
	s.idle(120);
	CHECK(!s.w->floatLogs.empty(), "no logs\n");
	FloatLog* lg = s.w->floatLogs[0].get();
	float y = lg->body->GetPosition().y;
	std::printf("    log centre y after 2 s: %.2f (surface 11.0)\n", y);
	CHECK(y > 10.9f && y < 11.3f, "log does not float\n");
	s.w->placePlayer(lg->body->GetPosition().x, y - 0.6f);
	s.idle(60);
	std::printf("    with Pip: log y %.2f, Pip grounded %d state %d\n", lg->body->GetPosition().y, s.p().grounded,
	            (int)s.p().state);
	CHECK(s.p().grounded && s.p().state == PState::Ground, "Pip should stand on the log\n");
}

void testMushroomAndBeetle() {
	std::printf("[L5] mushroom bounce heights, beetle stomp and beetle side contact\n");
	Sim s(5);
	// mushroom at (12,36); drop onto it from 3 tiles up
	s.w->placePlayer(12 * 0.5f + 0.25f, 16.5f);
	float minY = 100.f;
	bool bounced = false;
	for (int i = 0; i < 90; ++i) {
		s.step();
		if (s.p().vel().y < -9.f) bounced = true;
		if (bounced) minY = std::fmin(minY, s.p().bottom());
	}
	float h = (18.f - minY) / 0.5f;
	std::printf("    small bounce: %.2f tiles above the cap (design 4.0)\n", h);
	CHECK(bounced && std::fabs(h - 4.f) < 0.6f, "small bounce %.2f\n", h);
	s.w->placePlayer(12 * 0.5f + 0.25f, 16.5f);
	minY = 100.f;
	bounced = false;
	for (int i = 0; i < 90; ++i) {
		s.step(0, true);
		if (s.p().vel().y < -9.f) bounced = true;
		if (bounced) minY = std::fmin(minY, s.p().bottom());
	}
	h = (18.f - minY) / 0.5f;
	std::printf("    big bounce (Space held): %.2f tiles (design 6.76)\n", h);
	CHECK(bounced && std::fabs(h - 6.76f) < 0.8f, "big bounce %.2f\n", h);

	// beetle at (20,36): drop on it
	Sim b(5);
	b.idle(5);
	b2Vec2 bp = b.w->beetles[0]->body->GetPosition();
	b.w->placePlayer(bp.x, bp.y - 1.5f);
	for (int i = 0; i < 40; ++i) b.step();
	int deadBeetles = 0;
	for (auto& e : b.w->beetles) deadBeetles += e->dead;
	CHECK(deadBeetles == 1 && b.p().alive(), "stomp failed (dead beetles %d, alive %d)\n", deadBeetles,
	      b.p().alive());
	// walk into a beetle
	Sim c(5);
	c.idle(5);
	bp = c.w->beetles[0]->body->GetPosition();
	c.w->placePlayer(bp.x - 2.f, bp.y - 0.2f);
	bool died = runUntil(c, 240, [](Sim&, int) { Input in; in.dir = 1; return in; },
	                     [](Sim& s) { return !s.p().alive(); });
	CHECK(died, "walking into a beetle should kill\n");
	c.idle(70);
	CHECK(c.p().alive() && c.w->deaths == 1, "respawn after death failed\n");
}

void testCrateSeesaw() {
	std::printf("[L6] crate push and see-saw puzzle\n");
	Sim s(6);
	s.idle(60);
	Crate* cr = nullptr; // crate at (10,28)
	for (auto& c : s.w->crates)
		if (c->body->GetPosition().x < 6.f) cr = c.get();
	float cx0 = cr->body->GetPosition().x;
	s.placeOnTile(6, 28);
	for (int i = 0; i < 60; ++i) s.step(1);
	float moved = cr->body->GetPosition().x - cx0;
	std::printf("    crate pushed %.2f m in 1 s (speed %.2f)\n", moved, cr->body->GetLinearVelocity().x);
	CHECK(moved > 1.f, "crate did not move\n");
	// see-saw 2 (limit 40) should be tilted right-down by its crate
	Seesaw* ss = nullptr;
	for (auto& q : s.w->seesaws)
		if (q->limitDeg > 30.f) ss = q.get();
	CHECK(ss != nullptr, "40-degree see-saw not found\n");
	if (ss) {
		float a = ss->body->GetAngle() * 180.f / b2_pi;
		std::printf("    see-saw 2 angle with its crate: %.1f deg\n", a);
		CHECK(a > 30.f, "crate does not hold the see-saw down\n");
	}
}

void testWindCrumbleUpdraft() {
	std::printf("[L7] tailwind jump, updraft, crumbling ledges\n");
	Sim s(7);
	// tailwind jump over the 8-wide gap (x 31..38, wind '>' columns)
	s.placeOnTile(26, 44);
	s.idle(10);
	bool ok = runUntil(
	    s, 60 * 5,
	    [](Sim& s, int) {
		    Input in;
		    in.dir = 1;
		    bool jump = s.p().grounded && s.pos().x > 31 * 0.5f - 0.15f && s.pos().x < 16.f;
		    in.jumpPressed = jump;
		    in.jumpHeld = true;
		    return in;
	    },
	    [](Sim& s) { return s.p().grounded && s.pos().x > 39 * 0.5f; });
	std::printf("    tailwind gap crossed=%d x=%.2f deaths=%d\n", ok, s.pos().x, s.w->deaths);
	CHECK(ok && s.w->deaths == 0, "tailwind jump failed\n");

	Sim u(7);
	// updraft 1 at columns 62..64 rows 31..44
	u.w->placePlayer(63 * 0.5f + 0.25f, 44 * 0.5f);
	float minY = 100.f;
	for (int i = 0; i < 240; ++i) {
		u.step(0);
		minY = std::fmin(minY, u.p().bottom());
	}
	std::printf("    updraft 1 lifted feet to row %.1f (column top row 31)\n", minY / 0.5f);
	CHECK(minY / 0.5f < 31.f, "updraft too weak\n");

	Sim c(7);
	// crumble ledge '%%%' at x=75..77 row 31
	c.w->placePlayer(76 * 0.5f + 0.25f, 31 * 0.5f - 0.35f);
	c.idle(10);
	CHECK(c.w->crumbles.size() > 0, "no crumbles\n");
	int shaking = 0;
	for (auto& k : c.w->crumbles) shaking += k->state == Crumble::Shaking;
	CHECK(shaking >= 1, "crumble not triggered\n");
	c.idle(40);
	int fallen = 0;
	for (auto& k : c.w->crumbles) fallen += k->state == Crumble::Falling || k->state == Crumble::Gone;
	CHECK(fallen >= 1, "crumble did not fall\n");
	c.idle(260);
	int back = 0;
	for (auto& k : c.w->crumbles) back += k->state == Crumble::Solid || k->state == Crumble::Returning;
	CHECK(back == (int)c.w->crumbles.size(), "crumbles did not return\n");
}

void testMoversAndFallingLog() {
	std::printf("[L8] lily-pad mover carries Pip, hanging log falls\n");
	Sim s(8);
	Mover* m = s.w->movers[0].get();
	b2Vec2 m0 = m->body->GetPosition();
	s.w->placePlayer(m0.x, m0.y - 0.6f);
	s.idle(60 * 2);
	std::printf("    mover moved %.2f m, Pip dx %.2f, grounded %d\n", m->body->GetPosition().x - m0.x,
	            s.pos().x - m0.x, s.p().grounded);
	CHECK(s.p().grounded && std::fabs(s.pos().x - m->body->GetPosition().x) < 0.6f, "Pip not carried\n");

	Sim f(8);
	FallLog* lg = f.w->fallLogs[0].get(); // x=64..67 row 18
	f.w->placePlayer(lg->body->GetPosition().x, lg->body->GetPosition().y + 2.0f);
	f.idle(40);
	CHECK(lg->state != FallLog::Hanging, "log not triggered\n");
	CHECK(!f.p().alive() || f.w->deaths > 0, "standing under a falling log should be lethal\n");
}

void testPulleys() {
	std::printf("[L9] pulley lifts\n");
	Sim s(9);
	s.idle(60);
	Lift* l1 = s.w->lifts[0].get();
	float y0 = l1->body->GetPosition().y;
	CHECK(std::fabs(l1->body->GetLinearVelocity().y) < 0.05f, "lift 1 not at rest\n");
	std::printf("    lift 1 at rest y=%.2f\n", y0);
	// step on lift 1
	s.w->placePlayer(l1->body->GetPosition().x, y0 - 0.6f);
	s.idle(300);
	float drop = (l1->body->GetPosition().y - y0) / 0.5f;
	std::printf("    lift 1 descended %.1f tiles with Pip (design 18); Pip alive %d\n", drop, s.p().alive());
	CHECK(drop > 16.f && s.p().alive(), "lift 1 did not bring Pip down\n");

	Sim c(9);
	c.idle(60);
	Lift* l3 = nullptr;
	Lift* l4 = nullptr;
	for (auto& l : c.w->lifts) {
		if (l->w == 6) l4 = l.get();
		if (l->partner && l->partner->w == 6) l3 = l.get();
	}
	CHECK(l3 && l4, "lifts 3/4 not found\n");
	if (!l3 || !l4) return;
	float y3 = l3->body->GetPosition().y, y4 = l4->body->GetPosition().y;
	c.w->placePlayer(l3->body->GetPosition().x, y3 - 0.6f);
	c.idle(120);
	std::printf("    Pip alone on lift 3: lift 3 moved %.2f tiles, lift 4 %.2f tiles\n",
	            (l3->body->GetPosition().y - y3) / 0.5f, (l4->body->GetPosition().y - y4) / 0.5f);
	CHECK(std::fabs(l4->body->GetPosition().y - y4) < 0.3f, "Pip alone should not move lift 4\n");
	// drop the crate on lift 3
	c.w->crates[0]->body->SetTransform(l3->body->GetPosition() - b2Vec2(-0.3f, 0.6f), 0.f);
	c.idle(240);
	float rise = (y4 - l4->body->GetPosition().y) / 0.5f;
	std::printf("    crate + Pip on lift 3: lift 4 rose %.2f tiles (design 6)\n", rise);
	CHECK(rise > 5.f, "lift 4 did not rise\n");
}

void testDeathAndRespawnReset() {
	std::printf("[core] death by thorns, respawn at checkpoint, objects reset\n");
	Sim s(3);
	// thorns at row 29 (x 35..48) under the first bridge; drop Pip on them
	s.w->placePlayer(40 * 0.5f, 13.5f);
	bool died = runUntil(s, 120, [](Sim&, int) { return Input(); }, [](Sim& s) { return !s.p().alive(); });
	CHECK(died, "thorns did not kill\n");
	s.idle(70);
	CHECK(s.p().alive(), "no respawn\n");
	b2Vec2 sp = s.w->spawnPoint();
	CHECK(std::fabs(s.pos().x - sp.x) < 0.3f, "respawned away from spawn point\n");
	// R restart
	int d = s.w->deaths;
	Input in;
	in.restartPressed = true;
	s.w->step(in);
	CHECK(s.w->deaths == d + 1, "R should count as a death\n");
}

} // namespace

int main(int argc, char** argv) {
	initDataDir(argc > 0 ? argv[0] : nullptr);
	testJumpFeel();
	testCoyoteAndBuffer();
	testOneWay();
	testLevel1Complete();
	testRope();
	testLevel2FirstPit();
	testBridge();
	testSwim();
	testLogs();
	testMushroomAndBeetle();
	testCrateSeesaw();
	testWindCrumbleUpdraft();
	testMoversAndFallingLog();
	testPulleys();
	testDeathAndRespawnReset();
	if (gFails) {
		std::printf("%d check(s) FAILED\n", gFails);
		return 1;
	}
	std::printf("All mechanics tests passed.\n");
	return 0;
}
