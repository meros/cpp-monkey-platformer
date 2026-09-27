#include "Level.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include <limits.h>
#include <unistd.h>

static const char* kBiomeNames[] = {"canopy", "grove", "gully", "river", "hollow",
                                    "ridge",  "cliffs", "swamp", "mill",  "storm"};

const char* biomeName(Biome b) { return kBiomeNames[static_cast<int>(b)]; }

bool biomeFromName(const std::string& s, Biome& out) {
	for (int i = 0; i < static_cast<int>(Biome::Count); ++i) {
		if (s == kBiomeNames[i]) {
			out = static_cast<Biome>(i);
			return true;
		}
	}
	return false;
}

int LevelData::count(char c) const {
	int n = 0;
	for (const auto& r : rows)
		for (char ch : r)
			if (ch == c) ++n;
	return n;
}

static std::string trim(const std::string& s) {
	size_t a = s.find_first_not_of(" \t\r");
	if (a == std::string::npos) return "";
	size_t b = s.find_last_not_of(" \t\r");
	return s.substr(a, b - a + 1);
}

static const std::string kLegend = ".#/\\=-^~{}<>AR|M%XOLFePECbGS0123456789";

static bool isLegend(char c) { return kLegend.find(c) != std::string::npos; }

static uint8_t waterCode(char c) {
	switch (c) {
	case '~': return W_STILL;
	case '{': return W_LEFT;
	case '}': return W_RIGHT;
	default: return W_NONE;
	}
}

static uint8_t windCode(char c) {
	switch (c) {
	case '<': return WI_LEFT;
	case '>': return WI_RIGHT;
	case 'A': return WI_UP;
	default: return WI_NONE;
	}
}

// Cells that may sit "inside" a region and leave a cosmetic hole (4.1 semantics).
static bool isOverlay(char c) {
	return c == 'b' || c == 'G' || c == 'L' || c == 'C' || c == 'S' || c == 'e' || c == 'X' ||
	       c == 'O' || (c >= '0' && c <= '9');
}

static void deriveRegions(LevelData& L) {
	L.water.assign(L.width * L.height, W_NONE);
	L.wind.assign(L.width * L.height, WI_NONE);
	for (int y = 0; y < L.height; ++y) {
		for (int x = 0; x < L.width; ++x) {
			char c = L.rows[y][x];
			L.water[y * L.width + x] = waterCode(c);
			L.wind[y * L.width + x] = windCode(c);
		}
	}
	// Fill overlay holes when the region continues on both sides (or meets a wall).
	for (int y = 0; y < L.height; ++y) {
		for (int x = 0; x < L.width; ++x) {
			if (!isOverlay(L.rows[y][x])) continue;
			int l = x - 1, r = x + 1;
			while (l >= 0 && isOverlay(L.rows[y][l])) --l;
			while (r < L.width && isOverlay(L.rows[y][r])) ++r;
			char cl = L.at(l, y), cr = L.at(r, y);
			auto pick = [&](uint8_t (*code)(char)) -> uint8_t {
				uint8_t a = code(cl), b = code(cr);
				if (a && b) return a;
				if (a && cr == '#') return a;
				if (b && cl == '#') return b;
				return 0;
			};
			L.water[y * L.width + x] = pick(waterCode);
			L.wind[y * L.width + x] = pick(windCode);
		}
	}
}

bool parseLevel(const std::string& text, LevelData& L, std::string& err) {
	std::istringstream in(text);
	std::string line;
	int lineNo = 0;
	bool haveSize = false, haveName = false, haveBiome = false, inMap = false, ended = false;
	while (std::getline(in, line)) {
		++lineNo;
		if (!line.empty() && line.back() == '\r') line.pop_back();
		if (inMap) {
			if (line == "end") {
				ended = true;
				break;
			}
			L.rows.push_back(line);
			continue;
		}
		if (line.empty() || line[0] == '#') continue;
		if (line == "map:") {
			inMap = true;
			continue;
		}
		size_t colon = line.find(':');
		if (colon == std::string::npos) {
			err = "line " + std::to_string(lineNo) + ": expected 'key: value'";
			return false;
		}
		std::string key = trim(line.substr(0, colon));
		std::string val = trim(line.substr(colon + 1));
		if (key == "name") {
			L.name = val;
			haveName = true;
		} else if (key == "biome") {
			if (!biomeFromName(val, L.biome)) {
				err = "unknown biome '" + val + "'";
				return false;
			}
			haveBiome = true;
		} else if (key == "size") {
			std::istringstream v(val);
			if (!(v >> L.width >> L.height)) {
				err = "bad size";
				return false;
			}
			haveSize = true;
		} else if (key == "wind") {
			std::istringstream v(val);
			std::string mode;
			v >> mode;
			if (mode == "steady") {
				L.gust = false;
			} else if (mode == "gust") {
				L.gust = true;
				if (!(v >> L.gustOn >> L.gustOff) || L.gustOn <= 0 || L.gustOff <= 0) {
					err = "bad gust timing";
					return false;
				}
			} else {
				err = "bad wind mode '" + mode + "'";
				return false;
			}
		} else if (key.size() == 5 && key.compare(0, 4, "obj ") == 0 && key[4] >= '0' &&
		           key[4] <= '9') {
			int d = key[4] - '0';
			ObjDef& o = L.objs[d];
			if (o.defined) {
				err = "obj " + std::to_string(d) + " defined twice";
				return false;
			}
			o.defined = true;
			std::istringstream v(val);
			v >> o.type;
			if (o.type != "mover" && o.type != "seesaw" && o.type != "pulley") {
				err = "unknown object type '" + o.type + "'";
				return false;
			}
			std::string kv;
			while (v >> kv) {
				size_t eq = kv.find('=');
				if (eq == std::string::npos) {
					err = "bad object parameter '" + kv + "'";
					return false;
				}
				o.params[kv.substr(0, eq)] = std::strtof(kv.c_str() + eq + 1, nullptr);
			}
		} else if (key == "sign") {
			L.signs.push_back(val);
		} else if (key == "card_in") {
			L.cardIn.push_back(val);
		} else if (key == "card_out") {
			L.cardOut.push_back(val);
		} else {
			err = "line " + std::to_string(lineNo) + ": unknown key '" + key + "'";
			return false;
		}
	}
	if (!haveName || !haveBiome || !haveSize) {
		err = "missing name/biome/size header";
		return false;
	}
	if (!inMap || !ended) {
		err = "missing map:/end";
		return false;
	}
	if (static_cast<int>(L.rows.size()) != L.height) {
		err = "row count " + std::to_string(L.rows.size()) + " != height " + std::to_string(L.height);
		return false;
	}
	for (size_t y = 0; y < L.rows.size(); ++y) {
		if (static_cast<int>(L.rows[y].size()) != L.width) {
			err = "row " + std::to_string(y) + " has length " + std::to_string(L.rows[y].size());
			return false;
		}
		for (size_t x = 0; x < L.rows[y].size(); ++x) {
			char c = L.rows[y][x];
			if (!isLegend(c)) {
				err = "illegal character '" + std::string(1, c) + "' at " + std::to_string(x) + "," +
				      std::to_string(y);
				return false;
			}
			if (c >= '0' && c <= '9') {
				ObjDef& o = L.objs[c - '0'];
				if (o.x >= 0) {
					err = "object marker " + std::string(1, c) + " used twice";
					return false;
				}
				o.x = static_cast<int>(x);
				o.y = static_cast<int>(y);
			}
		}
	}
	deriveRegions(L);
	return true;
}

bool loadLevelFile(const std::string& path, LevelData& out, std::string& err) {
	std::ifstream f(path, std::ios::binary);
	if (!f) {
		err = "cannot open " + path;
		return false;
	}
	std::stringstream ss;
	ss << f.rdbuf();
	out = LevelData();
	out.path = path;
	return parseLevel(ss.str(), out, err);
}

bool loadLevelIndex(int index, LevelData& out, std::string& err) {
	bool ok = loadLevelFile(levelFilePath(index), out, err);
	out.index = index;
	return ok;
}

static bool isSupport(char c) {
	return c == '#' || c == '=' || c == '-' || c == '/' || c == '\\' || c == '%' || c == 'M';
}

std::vector<std::string> validateLevel(const LevelData& L) {
	std::vector<std::string> errs;
	auto cell = [](int x, int y) { return "(" + std::to_string(x) + "," + std::to_string(y) + ")"; };
	if (L.width < 60 || L.width > 250 || L.height < 20 || L.height > 80)
		errs.push_back("size out of bounds");
	if (L.count('P') != 1) errs.push_back("need exactly one P");
	if (L.count('E') != 1) errs.push_back("need exactly one E");
	if (L.count('G') != 1) errs.push_back("need exactly one G");
	if (L.count('C') < 1) errs.push_back("need at least one C");
	if (L.count('S') != static_cast<int>(L.signs.size()))
		errs.push_back("S count " + std::to_string(L.count('S')) + " != sign lines " +
		               std::to_string(L.signs.size()));
	if (L.cardIn.size() > 3 || L.cardOut.size() > 3) errs.push_back("cards have more than 3 lines");
	for (int d = 0; d < 10; ++d) {
		const ObjDef& o = L.objs[d];
		if (o.defined && o.x < 0) errs.push_back("obj " + std::to_string(d) + " defined but unused");
		if (!o.defined && o.x >= 0) errs.push_back("marker " + std::to_string(d) + " not defined");
		if (o.defined && o.type == "pulley") {
			int p = static_cast<int>(o.get("partner", -1));
			if (p < 0 || p > 9 || !L.objs[p].defined || L.objs[p].type != "pulley" ||
			    static_cast<int>(L.objs[p].get("partner", -1)) != d)
				errs.push_back("pulley " + std::to_string(d) + " partner mismatch");
			if (o.params.find("top") == o.params.end())
				errs.push_back("pulley " + std::to_string(d) + " missing top");
		}
		if (o.defined && (o.type == "mover" || o.type == "seesaw" || o.type == "pulley") &&
		    o.get("w", 0) < 1)
			errs.push_back("obj " + std::to_string(d) + " missing width");
	}
	for (int y = 0; y < L.height; ++y) {
		for (int x = 0; x < L.width; ++x) {
			char c = L.at(x, y);
			if (c == '|' && L.at(x, y - 1) != '|' && L.at(x, y - 1) != 'R')
				errs.push_back("vine segment without anchor at " + cell(x, y));
			if (c == '-' && L.at(x - 1, y) != '-' && L.at(x - 1, y) != '#')
				errs.push_back("bridge not bounded on the left at " + cell(x, y));
			if (c == '-' && L.at(x + 1, y) != '-' && L.at(x + 1, y) != '#')
				errs.push_back("bridge not bounded on the right at " + cell(x, y));
			if (c == 'O') {
				for (int dy = -1; dy <= 1; ++dy)
					for (int dx = -1; dx <= 1; ++dx)
						if ((dx || dy) && L.solid(x + dx, y + dy))
							errs.push_back("boulder touches terrain at " + cell(x, y));
			}
			if ((c == 'P' || c == 'E' || c == 'C' || c == 'S' || c == 'e' || c == 'M') &&
			    !isSupport(L.at(x, y + 1)))
				errs.push_back(std::string("'") + c + "' without support at " + cell(x, y));
			if ((c == 'P' || c == 'E' || c == 'C') && L.solid(x, y - 1))
				errs.push_back(std::string("'") + c + "' without head room at " + cell(x, y));
		}
	}
	return errs;
}

// ---------------------------------------------------------------------------
// Data directory

static std::string gDataDir = "data";

static bool fileExists(const std::string& p) {
	std::ifstream f(p);
	return f.good();
}

void initDataDir(const char* argv0) {
	std::vector<std::string> candidates;
	if (const char* env = std::getenv("LEAFWIND_DATA")) candidates.push_back(env);
	candidates.push_back("data");
	std::string exeDir;
	char buf[PATH_MAX];
	ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
	if (n > 0) {
		buf[n] = 0;
		exeDir = buf;
	} else if (argv0) {
		exeDir = argv0;
	}
	size_t slash = exeDir.find_last_of('/');
	if (slash != std::string::npos) {
		exeDir = exeDir.substr(0, slash);
		candidates.push_back(exeDir + "/data");
		candidates.push_back(exeDir + "/../data");
		candidates.push_back(exeDir + "/../share/leafwind/data");
	}
	for (const auto& c : candidates) {
		if (fileExists(c + "/levels/level01.txt")) {
			gDataDir = c;
			return;
		}
	}
}

std::string dataPath(const std::string& rel) { return gDataDir + "/" + rel; }

std::string levelFilePath(int index) {
	char name[64];
	std::snprintf(name, sizeof(name), "levels/level%02d.txt", index);
	return dataPath(name);
}
