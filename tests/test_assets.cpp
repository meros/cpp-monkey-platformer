// Leafwind -- the few bundled files the game needs are present (everything else is generated).
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

int main() {
	std::vector<std::string> required = {
	    "data/apa_stand_right.tga", "data/apa_stand_left.tga",   "data/apa_walk_right_1.tga",
	    "data/apa_walk_right_2.tga", "data/apa_walk_left_1.tga",  "data/apa_walk_left_2.tga",
	    "data/apa_jump_right.tga",   "data/apa_jump_left.tga",    "data/fonts/DejaVuSans.ttf",
	    "data/fonts/DejaVuSans-Bold.ttf", "data/fonts/LICENSE-DejaVu.txt",
	};
	for (int i = 1; i <= 10; ++i) {
		char buf[64];
		std::snprintf(buf, sizeof(buf), "data/levels/level%02d.txt", i);
		required.push_back(buf);
	}
	// Retired prototype art must stay gone.
	std::vector<std::string> removed = {"data/groundtile.tga", "data/rope.tga", "data/level.txt",
	                                    "data/smb3_mario_sheet.png", "alleg42.dll"};
	int fails = 0;
	for (const auto& f : required) {
		if (!std::ifstream(f).good()) {
			std::printf("  FAIL: missing %s\n", f.c_str());
			++fails;
		}
	}
	for (const auto& f : removed) {
		if (std::ifstream(f).good()) {
			std::printf("  FAIL: obsolete file still present: %s\n", f.c_str());
			++fails;
		}
	}
	std::printf(fails ? "%d asset check(s) failed\n" : "All asset checks passed (%d files).\n",
	            fails ? fails : static_cast<int>(required.size()));
	return fails ? 1 : 0;
}
