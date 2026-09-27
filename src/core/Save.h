// Leafwind -- progress save file (docs/DESIGN.md 7.3).
#pragma once

#include "Level.h"

#include <string>

struct LevelRecord {
	bool unlocked = false;
	bool done = false;
	int bananas = 0;   // best
	int total = 0;
	bool fig = false;  // ever collected
	int deaths = 0;    // best (fewest) among completions
	float time = 0.f;  // best (fastest) completion, 0 = none
};

struct RunResult {
	int bananas = 0, total = 0, deaths = 0;
	bool fig = false;
	float time = 0.f;
};

struct NewBest {
	bool bananas = false, fig = false, deaths = false, time = false, first = false;
};

class SaveData {
public:
	SaveData();
	LevelRecord levels[NUM_LEVELS + 1]; // 1-based
	float volume = 0.8f;
	bool music = true;
	bool existed = false;               // a save file was found on load

	bool load();
	bool save() const;
	std::string path() const { return myPath; }
	void setPath(const std::string& p) { myPath = p; }

	// Merge a completed run; unlocks the next level. Returns which values are new bests.
	NewBest recordCompletion(int level, const RunResult& r);
	int firstUnfinished() const;
	bool anyProgress() const;
	int totalBananas() const;
	int totalFigs() const;

private:
	std::string myPath;
};

std::string defaultSavePath();
