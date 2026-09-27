// Leafwind -- level file format (docs/DESIGN.md section 4.1) and validator (8.2).
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

constexpr int NUM_LEVELS = 10;

enum class Biome { Canopy, Grove, Gully, River, Hollow, Ridge, Cliffs, Swamp, Mill, Storm, Count };

const char* biomeName(Biome b);
bool biomeFromName(const std::string& s, Biome& out);

struct ObjDef {
	bool defined = false;
	std::string type;                  // mover | seesaw | pulley
	std::map<std::string, float> params;
	int x = -1, y = -1;                // marker cell (filled from the map)
	float get(const std::string& key, float def) const {
		auto it = params.find(key);
		return it == params.end() ? def : it->second;
	}
};

// Derived per-cell region codes
enum Water : uint8_t { W_NONE = 0, W_STILL = 1, W_LEFT = 2, W_RIGHT = 3 };
enum Wind : uint8_t { WI_NONE = 0, WI_LEFT = 1, WI_RIGHT = 2, WI_UP = 3 };

struct LevelData {
	int index = 0;                     // 1-based
	std::string path;
	std::string name;
	Biome biome = Biome::Canopy;
	int width = 0, height = 0;
	bool gust = false;
	float gustOn = 0.f, gustOff = 0.f;
	ObjDef objs[10];
	std::vector<std::string> signs, cardIn, cardOut;
	std::vector<std::string> rows;
	std::vector<uint8_t> water, wind; // width*height, derived at load

	char at(int x, int y) const {
		if (x < 0 || x >= width) return '#';
		if (y < 0 || y >= height) return '.';
		return rows[y][x];
	}
	bool solid(int x, int y) const { return at(x, y) == '#'; }
	uint8_t waterAt(int x, int y) const {
		if (x < 0 || x >= width || y < 0 || y >= height) return W_NONE;
		return water[y * width + x];
	}
	uint8_t windAt(int x, int y) const {
		if (x < 0 || x >= width || y < 0 || y >= height) return WI_NONE;
		return wind[y * width + x];
	}
	int count(char c) const;
	float widthM() const { return width * 0.5f; }
	float heightM() const { return height * 0.5f; }
};

// Parse from text. Returns false and fills err on a hard format error.
bool parseLevel(const std::string& text, LevelData& out, std::string& err);
bool loadLevelFile(const std::string& path, LevelData& out, std::string& err);
bool loadLevelIndex(int index, LevelData& out, std::string& err);

// Rules 1-4 of section 8.2. Returns a list of errors (empty = ok).
std::vector<std::string> validateLevel(const LevelData& lvl);

// Data directory resolution ("data/..." relative paths).
void initDataDir(const char* argv0);
std::string dataPath(const std::string& rel);
std::string levelFilePath(int index);
