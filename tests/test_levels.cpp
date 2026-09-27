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

struct Route {
	int level;
	bool (*fn)(RouteBot&);
};

const Route kRoutes[] = {{1, level1}, {2, level2}, {3, level3}, {4, level4}};

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
