#include "Sprite.h"

#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

static bool loadAndCheck(const char* path, const char* name) {
	Sprite sprite;
	bool loaded = sprite.LoadTGA(path);
	if (!loaded) {
		fprintf(stderr, "  FAIL: could not load sprite: %s (%s)\n", name, path);
	} else {
		printf("  PASS: loaded %s\n", name);
	}
	return loaded;
}

static bool loadMirrorAndCheck(const char* path, const char* name) {
	Sprite sprite;
	bool loaded = sprite.LoadTGA(path);
	if (!loaded) {
		fprintf(stderr, "  FAIL: could not load sprite for mirroring: %s (%s)\n", name, path);
		return false;
	}
	sprite.Mirror();
	printf("  PASS: loaded + mirrored %s\n", name);
	return true;
}

int main() {
	printf("Sprite animation tests (all Player states):\n");

	bool allOk = true;

	// Exact reproduction of Player::Player sprite loading:
	// Stand
	allOk &= loadAndCheck("data/apa_stand_right.tga", "stand right");
	allOk &= loadAndCheck("data/apa_stand_left.tga", "stand left");

	// Break (reuses stand sprites)
	allOk &= loadAndCheck("data/apa_stand_right.tga", "break right (stand)");
	allOk &= loadAndCheck("data/apa_stand_left.tga", "break left (stand)");

	// Walk right
	allOk &= loadAndCheck("data/apa_walk_right_1.tga", "walk right 1");
	allOk &= loadAndCheck("data/apa_walk_right_2.tga", "walk right 2");

	// Walk left
	allOk &= loadAndCheck("data/apa_walk_left_1.tga", "walk left 1");
	allOk &= loadAndCheck("data/apa_walk_left_2.tga", "walk left 2");

	// Jump
	allOk &= loadAndCheck("data/apa_jump_right.tga", "jump right");
	allOk &= loadAndCheck("data/apa_jump_left.tga", "jump left");

	// Climb (reuses jump sprites)
	allOk &= loadAndCheck("data/apa_jump_left.tga", "climb left (jump)");
	allOk &= loadAndCheck("data/apa_jump_right.tga", "climb right (jump)");

	// Environment
	allOk &= loadAndCheck("data/groundtile.tga", "ground tile");
	allOk &= loadAndCheck("data/rope.tga", "rope");

	assert(allOk && "Some sprites failed to load");
	printf("All sprite tests passed.\n");
	return 0;
}
