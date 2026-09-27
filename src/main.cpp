#include "core/Headless.h"
#include "core/Level.h"

#include <cstdlib>
#include <cstring>
#include <string>

int main(int argc, char** argv) {
	initDataDir(argv[0]);
	for (int i = 1; i < argc; ++i) {
		if (!std::strcmp(argv[i], "--validate-levels")) return runValidateLevels();
		if (!std::strcmp(argv[i], "--test-physics")) {
			int lvl = (i + 1 < argc) ? std::atoi(argv[i + 1]) : 0;
			return runPhysicsTest(lvl);
		}
	}
	return 0;
}
