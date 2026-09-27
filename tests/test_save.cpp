// Leafwind -- save file round trip and best-of bookkeeping (docs/DESIGN.md 7.3).
#include "core/Save.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

static int fails = 0;
#define CHECK(c)                                                                                             \
	do {                                                                                                     \
		if (!(c)) {                                                                                          \
			std::printf("  FAIL: %s (line %d)\n", #c, __LINE__);                                            \
			++fails;                                                                                         \
		}                                                                                                    \
	} while (0)

int main() {
	std::string path = std::string(std::getenv("TMPDIR") ? std::getenv("TMPDIR") : "/tmp") + "/leafwind-test-save.txt";
	std::remove(path.c_str());
	SaveData s;
	s.setPath(path);
	CHECK(!s.load());
	CHECK(s.levels[1].unlocked && !s.levels[2].unlocked);
	CHECK(s.firstUnfinished() == 1);

	RunResult r{20, 28, 3, false, 90.f};
	NewBest nb = s.recordCompletion(1, r);
	CHECK(nb.first);
	CHECK(s.levels[1].done && s.levels[2].unlocked);
	CHECK(s.firstUnfinished() == 2);
	// a worse run keeps the bests, a better one updates each independently
	nb = s.recordCompletion(1, RunResult{25, 28, 5, true, 120.f});
	CHECK(nb.bananas && nb.fig && !nb.deaths && !nb.time);
	CHECK(s.levels[1].bananas == 25 && s.levels[1].deaths == 3 && s.levels[1].time == 90.f && s.levels[1].fig);
	nb = s.recordCompletion(1, RunResult{10, 28, 1, false, 60.f});
	CHECK(nb.deaths && nb.time && !nb.bananas);
	s.volume = 0.5f;
	s.music = false;
	CHECK(s.save());

	SaveData t;
	t.setPath(path);
	CHECK(t.load());
	CHECK(t.existed);
	CHECK(t.levels[1].done && t.levels[1].bananas == 25 && t.levels[1].deaths == 1 && t.levels[1].fig);
	CHECK(t.levels[1].time > 59.9f && t.levels[1].time < 60.1f);
	CHECK(t.levels[2].unlocked && !t.levels[3].unlocked);
	CHECK(t.volume == 0.5f && !t.music);
	CHECK(t.totalBananas() == 25 && t.totalFigs() == 1);
	{
		std::ifstream f(path);
		std::string first;
		std::getline(f, first);
		CHECK(first == "leafwind-save 1");
	}
	std::remove(path.c_str());
	std::printf(fails ? "%d save check(s) failed\n" : "All save tests passed.\n", fails);
	return fails ? 1 : 0;
}
