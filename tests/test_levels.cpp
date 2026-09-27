// Leafwind -- full scripted playthroughs of every level on the real physics (no teleports).
#include "RouteBot.h"

#include <cstring>

namespace {

// Reactive bot: run right, jump when a wall or a gap is ahead.
Input runRightBot(RouteBot& b) {
	Input in;
	in.dir = 1;
	int x = static_cast<int>(std::floor(b.tx())), y = static_cast<int>(std::floor(b.feet() - 0.01f));
	bool wall = b.L.solid(x + 1, y) || b.L.solid(x + 1, y - 1);
	bool gap = !b.L.solid(x + 1, y + 1) && !b.L.solid(x + 2, y + 1) && b.L.at(x + 1, y + 1) != '=';
	bool jump = b.grounded() && (wall || gap);
	in.jumpPressed = jump;
	in.jumpHeld = jump || (!b.grounded() && b.p().vel().y < 0.f);
	return in;
}

bool level1(RouteBot& b) {
	b.until("run to exit", 60 * 60, [&] { return runRightBot(b); }, [&] { return b.w->complete; });
	return b.ok;
}

bool level2(RouteBot& b) {
	// vine over the 6-wide pit
	b.jumpAt(14.3f, 1, 20, 1, [&] { return b.onRope(); });
	b.swingRelease(1, 0.5f, [&] { return b.onGroundRightOf(20.5f); });
	// two vines over the 15-wide pit
	b.jumpAt(34.2f, 1, 20, 1, [&] { return b.onRope(); });
	b.swingRelease(1, 0.5f, [&] { return b.onRope() && b.tx() > 42.f; });
	b.swingRelease(1, 0.5f, [&] { return b.onGroundRightOf(49.5f); });
	// long vine beside the 10-tall wall: climb, then swing onto the top
	b.walkTo(54.f);
	b.jumpAt(56.5f, 1, 20, 1, [&] { return b.onRope(); });
	b.climbTo(3);
	b.swingRelease(1, 0.5f, [&] { return b.onGroundRightOf(62.5f) && b.feet() < 17.5f; });
	// ravine: three vines
	b.walkTo(78.f);
	b.jumpAt(80.2f, 1, 20, 1, [&] { return b.onRope(); });
	b.swingRelease(1, 0.5f, [&] { return b.onRope() && b.tx() > 90.f; });
	b.swingRelease(1, 0.5f, [&] { return b.onRope() && b.tx() > 98.f; });
	b.swingRelease(1, 0.5f, [&] { return b.onGroundRightOf(109.5f); });
	b.until("exit", 900, [&] { return RouteBot::hold(1); }, [&] { return b.w->complete; });
	return b.ok;
}

bool level3(RouteBot& b) {
	b.walkTo(21.5f);                                                                 // safe bridge
	b.jumpAt(21.8f, 1, 12, 1, [&] { return b.onGroundRightOf(23.f); });              // +1 step
	b.walkTo(47.f);                                                                  // 14-plank bridge
	b.walkTo(51.f);
	b.jumpAt(52.3f, 1, 14, 1, [&] { return b.onGroundRightOf(56.f); });              // thorn hop
	b.stopAt(57.6f);
	b.jumpAt(NAN, 1, 30, 1, [&] { return b.grounded() && b.feet() < 23.5f; });       // +3 onto block A
	b.stopAt(61.5f);
	b.jumpAt(NAN, 1, 30, 1, [&] { return b.grounded() && b.feet() < 20.5f; });       // +3 onto rock B
	b.stopAt(65.5f);
	b.jumpAt(NAN, 1, 30, 1, [&] { return b.grounded() && b.feet() < 17.5f; });       // +3 onto rock C
	b.stopAt(69.4f);
	b.jumpAt(NAN, 1, 20, 1, [&] { return b.grounded() && b.tx() > 70.5f; });         // onto the cliff
	b.walkTo(105.f);                                                                 // 16-plank bridge
	b.jumpAt(108.4f, 1, 14, 1, [&] { return b.onGroundRightOf(112.f); });            // thorn hop
	b.jumpAt(113.5f, 1, 14, 1, [&] { return b.onGroundRightOf(117.f); });            // thorn hop
	b.until("exit", 900, [&] { return RouteBot::hold(1); }, [&] { return b.w->complete; });
	return b.ok;
}

bool level4(RouteBot& b) {
	b.walkTo(13.8f);
	b.crossRight(40.5f);   // river 1: logs or a swim
	b.walkTo(54.f);
	b.crossRight(87.5f);   // river 2: hop the drifting logs before the falls
	b.walkTo(99.f);
	b.crossRight(135.5f);  // the still lake
	b.until("exit", 900, [&] { return RouteBot::hold(1); }, [&] { return b.w->complete; });
	return b.ok;
}

bool level5(RouteBot& b) {
	b.stopAt(10.6f);
	b.bounce(12.f, false, 15.f, [&] { return b.onGroundRightOf(13.5f); });          // little bounce over M
	b.passBeetles(26.6f);                                                           // beetle patrol
	b.stopAt(26.8f);
	b.bounce(28.f, true, 33.f, [&] { return b.grounded() && b.feet() < 30.5f; });   // big bounce up the wall
	b.passBeetles(49.5f);                                                           // plateau beetle
	b.until("drop into the hollow", 240, [&] { return RouteBot::hold(1); }, [&] { return b.grounded() && b.feet() > 37.5f; });
	b.idle(10);
	b.bounce(54.f, true, 58.5f, [&] { return b.grounded() && b.feet() < 32.5f; });  // onto the pillar
	b.stopAt(57.8f);
	b.bounce(59.f, true, 66.f, [&] { return b.grounded() && b.feet() < 26.5f; });   // up to the branch
	b.stopAt(67.5f);
	b.jumpAt(NAN, 1, 16, 1, [&] { return b.onGroundRightOf(71.f); });              // onto the upper plateau
	b.stopAt(78.6f);
	b.bounce(80.f, true, 84.6f, [&] { return b.grounded() && b.feet() < 20.5f; }); // mushroom -> branch
	b.stopAt(84.8f);
	b.bounce(86.f, true, 94.f, [&] { return b.grounded() && b.feet() < 14.5f; });   // branch mushroom -> tower
	b.until("exit", 600, [&] { return RouteBot::hold(1); }, [&] { return b.w->complete; });
	return b.ok;
}

bool level6(RouteBot& b) {
	Crate* first = nullptr;
	for (auto& c : b.w->crates)
		if (c->body->GetPosition().x < 8.f) first = c.get();
	// push the crate to the wall, use it as a step
	b.until("push crate", 600, [&] { return RouteBot::hold(1); },
	        [&] { return first->body->GetPosition().x / cfg::TILE > 15.3f && std::fabs(first->body->GetLinearVelocity().x) < 0.05f; });
	b.idle(10);
	b.jumpAt(NAN, 1, 14, 1, [&] { return b.onKind(Kind::Crate); });
	b.idle(5);
	b.jumpAt(NAN, 1, 40, 1, [&] { return b.grounded() && b.feet() < 25.5f; });
	// tilting see-saw bridge over the pit
	b.walkTo(26.5f);
	b.until("drop", 120, [&] { return RouteBot::hold(1); }, [&] { return b.grounded() && b.feet() > 28.5f; });
	b.stopAt(29.2f);
	b.jumpAt(NAN, 1, 12, 1, [&] { return b.onKind(Kind::Seesaw); });
	b.until("cross see-saw", 600, [&] { return RouteBot::hold(1); },
	        [&] { return b.grounded() && b.tx() > 37.5f && !b.onKind(Kind::Seesaw); });
	// the see-saw puzzle: walk under the raised end, board the low end right of the crate
	Seesaw* ss = nullptr;
	for (auto& q : b.w->seesaws)
		if (q->limitDeg > 30.f) ss = q.get();
	Crate* pc = nullptr;
	for (auto& c : b.w->crates)
		if (std::fabs(c->body->GetPosition().x - ss->pivot.x) < 2.f) pc = c.get();
	auto localX = [&](b2Body* body) { return ss->body->GetLocalPoint(body->GetPosition()).x; };
	b.stopAt(55.5f);
	b.jumpAt(NAN, -1, 10, -1, [&] { return b.onKind(Kind::Seesaw); }, 240);
	// push the crate up and over the pivot, then all the way down to the far lip
	b.until("push crate over", 900, [&] { return RouteBot::hold(-1); },
	        [&] { return ss->body->GetAngle() < 0.f && localX(pc->body) < -1.1f; });
	b.idle(20);
	// run up to the raised end and jump for the branch
	b.until("run up", 300, [&] { return RouteBot::hold(1); },
	        [&] { return b.onKind(Kind::Seesaw) && ss->body->GetLocalPoint(b.p().pos()).x > 1.25f; });
	b.jumpAt(NAN, 1, 30, 1, [&] { return b.grounded() && b.feet() < 21.5f; });
	b.stopAt(56.8f);
	b.jumpAt(NAN, 1, 30, 1, [&] { return b.grounded() && b.feet() < 18.5f; });      // upper branch
	b.jumpAt(NAN, 1, 16, 1, [&] { return b.grounded() && b.tx() > 60.3f; });        // wall top
	// shelf with two crates and another 4-tall wall
	b.walkTo(75.5f);
	b.until("drop", 120, [&] { return RouteBot::hold(1); }, [&] { return b.grounded() && b.feet() > 23.5f; });
	b.until("push crates", 900, [&] { return RouteBot::hold(1); },
	        [&] { return b.tx() > 90.f && std::fabs(b.p().vel().x) < 0.05f; });
	b.idle(10);
	for (int k = 0; k < 3 && b.feet() > 21.f; ++k) {
		b.jumpAt(NAN, 1, 40, 1, [&] { return b.grounded() && b.p().vel().y >= -0.01f; });
		b.idle(5);
	}
	b.stopAt(105.f);
	// see-saw bridge over the thorns: running jump onto its raised end
	b.jumpAt(108.4f, 1, 30, 1, [&] { return b.onKind(Kind::Seesaw); });
	b.until("cross see-saw", 600, [&] { return RouteBot::hold(1); },
	        [&] { return b.grounded() && b.tx() > 118.5f && !b.onKind(Kind::Seesaw); });
	b.passBeetles(138.f);
	b.until("exit", 600, [&] { return RouteBot::hold(1); }, [&] { return b.w->complete; });
	return b.ok;
}

bool supportTile(const LevelData& L, int x, int y) {
	char c = L.at(x, y);
	return c == '#' || c == '%' || c == '=' || c == '/' || c == '\\';
}

bool level7(RouteBot& b) {
	// tailwind jump over the 8-wide gap
	b.stopAt(29.f);
	b.jumpAt(30.85f, 1, 30, 1, [&] { return b.onGroundRightOf(39.f); });
	// updraft 1: step in, rise, steer onto the ledge
	b.stopAt(62.4f);
	int uf = 0;
	b.until("ride updraft 1", 600, [&] {
		Input in = RouteBot::hold(b.feet() < 30.95f ? 1 : 0, uf < 10);
		in.jumpPressed = uf++ == 0;
		return in;
	}, [&] { return b.grounded() && b.feet() < 31.5f && b.tx() > 65.f; });
	// crumbling ledges against the wind: keep moving
	b.walkTo(70.f);
	int air = 0;
	b.until("crumbles", 900, [&] {
		Input in = RouteBot::hold(1);
		if (b.grounded()) {
			air = 0;
			int x = static_cast<int>(std::floor(b.tx() + 0.5f)), y = static_cast<int>(std::floor(b.feet() + 0.1f));
			if (!supportTile(b.L, x, y)) {
				in.jumpPressed = true;
				in.jumpHeld = true;
			}
		} else {
			in.jumpHeld = ++air < 25;
		}
		return in;
	}, [&] { return b.grounded() && b.tx() > 95.2f; });
	// updraft 2 to the top plateau
	b.until("ride updraft 2", 900, [&] {
		return RouteBot::hold(b.feet() < 8.95f ? -1 : (b.tx() < 96.4f ? 1 : 0));
	}, [&] { return b.grounded() && b.feet() < 9.5f && b.tx() < 95.5f; });
	b.until("exit", 900, [&] { return RouteBot::hold(-1); }, [&] { return b.w->complete; });
	return b.ok;
}

bool level8(RouteBot& b) {
	auto mover = [&](int i) { return b.w->movers[i]->body; };
	// lily pads across pool 1
	b.stopAt(20.3f);
	b.hopOnto(mover(0), 1, 2.f, 5.5f);
	b.ride([&] { return b.w->movers[0]->body->GetLinearVelocity().x == 0.f && b.tx() > 30.f; });
	b.hopOnto(mover(1), 1, 2.f, 6.6f);
	b.ride([&] { return b.tx() > 45.5f; });
	b.until("to the bank", 300, [&] {
		Input in = RouteBot::hold(1, true);
		in.jumpPressed = b.grounded() && b.tx() > 45.5f;
		return in;
	}, [&] { return b.onGroundRightOf(51.f); });
	// run under the first hanging log before it lands
	b.walkTo(69.f);
	b.stopAt(68.4f);
	b.jumpAt(NAN, 1, 30, 1, [&] { return b.grounded() && b.feet() < 21.5f; });   // branch 1
	b.stopAt(72.8f);
	b.jumpAt(NAN, 1, 30, 1, [&] { return b.grounded() && b.feet() < 18.5f; });   // branch 2
	b.stopAt(77.3f);
	b.jumpAt(NAN, 1, 14, 1, [&] { return b.onKind(Kind::FallLog); });              // onto the hanging log
	FallLog* big = nullptr;
	for (auto& l : b.w->fallLogs)
		if (l->tiles == 8) big = l.get();
	b.ride([&] { return big->state == FallLog::Resting; });                       // it drops and bridges the pit
	b.walkTo(95.f);
	// pool 2: rising pad, branch, drifting pad
	b.stopAt(100.3f);
	b.hopOnto(mover(2), 1, 1.5f, 4.2f, 1.3f);
	b.ride([&] { return b.w->movers[2]->body->GetPosition().y / cfg::TILE < 18.4f; });
	b.jumpAt(NAN, 1, 20, 1, [&] { return b.grounded() && b.tx() > 107.f && b.feet() < 18.5f; });
	b.stopAt(110.2f);
	b.hopOnto(mover(3), 1, 1.f, 3.5f, 1.f);
	b.ride([&] { return b.tx() > 123.5f; });
	b.until("to the bank", 300, [&] {
		Input in = RouteBot::hold(1, true);
		in.jumpPressed = b.grounded();
		return in;
	}, [&] { return b.onGroundRightOf(126.f); });
	// gauntlet: three hanging logs and two beetles
	b.gauntlet(163.f);
	b.until("exit", 600, [&] { return RouteBot::hold(1); }, [&] { return b.w->complete; });
	return b.ok;
}

bool level9(RouteBot& b) {
	Lift* lift[7] = {nullptr};
	{
		int k = 1;
		for (auto& l : b.w->lifts) lift[k++] = l.get();
	}
	// (A) lift 1 takes Pip down 18 tiles
	b.walkTo(17.2f);
	b.until("ride lift 1 down", 600, [&] { return RouteBot::hold(0); },
	        [&] { return b.grounded() && b.feet() > 30.f && std::fabs(lift[1]->body->GetLinearVelocity().y) < 0.02f; });
	// (B) crate onto lift 3 raises lift 4 as a bridge
	Crate* crate = b.w->crates[0].get();
	b.until("push crate onto lift 3", 900, [&] { return RouteBot::hold(1); },
	        [&] { return crate->body->GetPosition().x / cfg::TILE > 46.85f; });
	b.until("step back", 60, [&] { return RouteBot::hold(-1); }, [&] { return b.tx() < 45.f; });
	b.until("lift 4 up", 600, [&] { return RouteBot::hold(0); },
	        [&] { return lift[4]->body->GetPosition().y / cfg::TILE < 30.5f; });
	b.stopAt(45.2f);
	b.jumpAt(NAN, 1, 14, 1, [&] { return b.onGroundRightOf(49.5f); });
	b.stopAt(49.6f);
	// lift 4 sinks under Pip's weight: run-jump, touch down on its far end, keep running
	b.jumpAt(54.5f, 1, 30, 1, [&] { return b.grounded() && b.tx() > 61.3f; });
	b.walkTo(69.f);
	// (C) boulder trap: clear the pit, sprint up the slope under the hanging log
	b.jumpAt(70.6f, 1, 20, 1, [&] { return b.grounded() && b.tx() > 75.8f; });
	b.until("up the slope", 600, [&] { return RouteBot::hold(1); }, [&] { return b.grounded() && b.tx() > 88.f; });
	// (D) push the boulder onto lift 6, run back to lift 5 and ride it up
	Boulder* heavy = nullptr;
	for (auto& bo : b.w->boulders)
		if (bo->body->GetPosition().x / cfg::TILE > 100.f) heavy = bo.get();
	b.until("push the boulder off", 600, [&] { return RouteBot::hold(1); },
	        [&] { return heavy->body->GetPosition().x / cfg::TILE > 117.4f; });
	b.until("back down the slope", 600, [&] { return RouteBot::hold(-1); }, [&] { return b.grounded() && b.tx() < 110.3f; });
	b.hopOnto(lift[5]->body, -1, 0.5f, 4.f, 3.1f);
	b.until("ride lift 5 up", 900, [&] {
		float diff = lift[5]->body->GetPosition().x / cfg::TILE - b.tx();
		return RouteBot::hold(diff > 0.3f ? 1 : (diff < -0.3f ? -1 : 0));
	}, [&] { return b.grounded() && b.feet() < 10.3f; });
	b.until("exit", 600, [&] { return RouteBot::hold(-1); }, [&] { return b.w->complete; });
	return b.ok;
}

bool level10(RouteBot& b) {
	// Stage 1: mushroom, three branches, a vine, the left branch, hollow 1
	b.stopAt(12.8f);
	b.bounce(14.f, true, 14.f, [&] { return b.grounded() && b.feet() < 52.5f; });
	b.stopAt(15.6f);
	b.jumpAt(NAN, 1, 30, 1, [&] { return b.grounded() && b.feet() < 49.5f; });
	b.stopAt(20.6f);
	b.jumpAt(NAN, 1, 30, 1, [&] { return b.grounded() && b.feet() < 46.5f; });
	b.stopAt(25.5f);
	b.jumpAt(NAN, 1, 20, 1, [&] { return b.onRope(); });
	b.swingRelease(1, 0.5f, [&] { return b.grounded() && b.tx() > 38.5f; });
	b.passBeetles(79.f, 1500);                                 // through hollow 1
	// Stage 2: bridge, three crumbles up the outer wall, the pad back left
	b.walkTo(99.5f);
	b.stopAt(100.4f);
	int cf = 0;
	b.until("crumbles up the wall", 900, [&] {
		Input in = RouteBot::hold(1, true);
		++cf;
		in.jumpPressed = b.grounded() && (cf < 3 || b.onKind(Kind::Crumble));
		return in;
	}, [&] { return b.grounded() && b.feet() < 35.5f && b.tx() > 113.f; });
	b.stopAt(113.5f);
	b.hopOnto(b.w->movers[0]->body, -1, 0.5f, 3.5f, 2.5f);
	b.ride([&] { return b.w->movers[0]->body->GetPosition().x / cfg::TILE < 81.6f; }, 2400);
	b.until("off the pad", 200, [&] { return RouteBot::hold(-1); }, [&] { return b.grounded() && b.tx() < 79.3f && !b.onKind(Kind::Mover); });
	b.passBeetles(40.f, 1500, -1);                             // through hollow 2
	// Stage 3: vine, branch, vine, wall ledge, crumbles, branch, pad, trunk top
	b.stopAt(38.8f);
	b.jumpAt(NAN, -1, 20, -1, [&] { return b.onRope(); });
	b.climbTo(8);
	b.swingRelease(-1, 0.5f, [&] { return b.grounded() && b.feet() < 27.5f; });
	b.stopAt(26.6f);
	b.jumpAt(NAN, -1, 20, -1, [&] { return b.onRope(); });
	b.climbTo(10);
	b.swingRelease(-1, 0.5f, [&] { return b.grounded() && b.tx() < 15.f && b.feet() < 22.5f; });
	b.stopAt(12.f);
	b.jumpAt(8.9f, -1, 40, -1, [&] { return b.grounded() && b.feet() < 19.5f; });  // crumble A (+3)
	b.jumpAt(NAN, 1, 30, 1, [&] { return b.grounded() && b.feet() < 16.5f; });    // crumble B (+3)
	b.jumpAt(NAN, 1, 30, 1, [&] { return b.grounded() && b.feet() < 13.5f; });    // branch (+3)
	b.stopAt(14.5f);
	b.hopOnto(b.w->movers[1]->body, 1, 1.f, 3.5f, 1.5f);
	b.ride([&] { return b.tx() > 26.5f; });
	b.until("onto the big branch", 200, [&] {
		Input in = RouteBot::hold(1, true);
		in.jumpPressed = b.grounded() && b.onKind(Kind::Mover);
		return in;
	}, [&] { return b.grounded() && b.tx() > 30.5f && !b.onKind(Kind::Mover); });
	b.passBeetles(52.5f);
	b.stopAt(52.8f);
	b.jumpAt(NAN, 1, 30, 1, [&] { return b.grounded() && b.feet() < 9.5f; });     // trunk top
	b.until("the golden leaf", 600, [&] { return RouteBot::hold(1); }, [&] { return b.w->complete; });
	return b.ok;
}

struct Route {
	int level;
	bool (*fn)(RouteBot&);
};

const Route kRoutes[] = {{1, level1}, {2, level2}, {3, level3}, {4, level4}, {5, level5}, {6, level6}, {7, level7}, {8, level8}, {9, level9}, {10, level10}};

} // namespace

int main(int argc, char** argv) {
	initDataDir(argc > 0 ? argv[0] : nullptr);
	bool verbose = false;
	int only = 0;
	for (int i = 1; i < argc; ++i) {
		if (!std::strcmp(argv[i], "-v")) verbose = true;
		else only = std::atoi(argv[i]);
	}
	int fails = 0;
	for (const Route& r : kRoutes) {
		if (only && r.level != only) continue;
		std::printf("[level %d]\n", r.level);
		RouteBot b(r.level, verbose);
		if (!r.fn(b) || !b.w->complete) {
			if (b.ok) b.fail("route ended without completing the level");
			++fails;
		}
		char name[32];
		std::snprintf(name, sizeof(name), "level %02d", r.level);
		b.report(name);
	}
	if (fails) {
		std::printf("%d route(s) FAILED\n", fails);
		return 1;
	}
	std::printf("All level routes completed.\n");
	return 0;
}
