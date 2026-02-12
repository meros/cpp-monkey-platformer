#include <cassert>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

static bool fileExists(const std::string& path) {
	std::ifstream f(path);
	return f.good();
}

int main() {
	printf("Asset tests:\n");

	std::vector<std::string> requiredAssets = {
		"data/level.txt",
		"data/apa_stand_right.tga",
		"data/apa_stand_left.tga",
		"data/apa_walk_right_1.tga",
		"data/apa_walk_right_2.tga",
		"data/apa_walk_left_1.tga",
		"data/apa_walk_left_2.tga",
		"data/apa_jump_right.tga",
		"data/apa_jump_left.tga",
		"data/groundtile.tga",
		"data/rope.tga",
	};

	bool allFound = true;
	for (const auto& asset : requiredAssets) {
		if (!fileExists(asset)) {
			fprintf(stderr, "  FAIL: missing asset: %s\n", asset.c_str());
			allFound = false;
		} else {
			printf("  PASS: found %s\n", asset.c_str());
		}
	}

	assert(allFound && "Some required assets are missing");
	printf("All asset tests passed.\n");
	return 0;
}
