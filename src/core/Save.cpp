#include "Save.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

static bool dirWritable(const std::string& dir) {
	std::string probe = dir + "/.leafwind_probe";
	FILE* f = std::fopen(probe.c_str(), "w");
	if (!f) return false;
	std::fclose(f);
	std::remove(probe.c_str());
	return true;
}

static void mkdirs(const std::string& path) {
	std::string cur;
	std::stringstream ss(path);
	std::string part;
	if (!path.empty() && path[0] == '/') cur = "/";
	while (std::getline(ss, part, '/')) {
		if (part.empty()) continue;
		cur += part + "/";
		mkdir(cur.c_str(), 0755);
	}
}

std::string defaultSavePath() {
	if (const char* p = std::getenv("LEAFWIND_SAVE")) return p;
	std::string base;
	if (const char* x = std::getenv("XDG_DATA_HOME")) base = x;
	else if (const char* h = std::getenv("HOME")) base = std::string(h) + "/.local/share";
	if (!base.empty()) {
		std::string dir = base + "/leafwind";
		mkdirs(dir);
		if (dirWritable(dir)) return dir + "/save.txt";
	}
	char buf[PATH_MAX];
	ssize_t n = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
	if (n > 0) {
		buf[n] = 0;
		std::string exe(buf);
		size_t slash = exe.find_last_of('/');
		if (slash != std::string::npos) return exe.substr(0, slash) + "/save.txt";
	}
	return "save.txt";
}

SaveData::SaveData() : myPath(defaultSavePath()) { levels[1].unlocked = true; }

bool SaveData::load() {
	std::ifstream f(myPath);
	if (!f) return false;
	std::string line;
	if (!std::getline(f, line) || line.rfind("leafwind-save", 0) != 0) return false;
	while (std::getline(f, line)) {
		std::istringstream in(line);
		std::string kind;
		in >> kind;
		if (kind == "level") {
			int idx = 0;
			in >> idx;
			if (idx < 1 || idx > NUM_LEVELS) continue;
			LevelRecord& r = levels[idx];
			std::string kv;
			while (in >> kv) {
				size_t eq = kv.find('=');
				if (eq == std::string::npos) continue;
				std::string k = kv.substr(0, eq);
				float v = std::strtof(kv.c_str() + eq + 1, nullptr);
				if (k == "unlocked") r.unlocked = v != 0.f;
				else if (k == "done") r.done = v != 0.f;
				else if (k == "bananas") r.bananas = static_cast<int>(v);
				else if (k == "total") r.total = static_cast<int>(v);
				else if (k == "fig") r.fig = v != 0.f;
				else if (k == "deaths") r.deaths = static_cast<int>(v);
				else if (k == "time") r.time = v;
			}
		} else if (kind == "options") {
			std::string kv;
			while (in >> kv) {
				size_t eq = kv.find('=');
				if (eq == std::string::npos) continue;
				std::string k = kv.substr(0, eq);
				float v = std::strtof(kv.c_str() + eq + 1, nullptr);
				if (k == "volume") volume = v;
				else if (k == "music") music = v != 0.f;
			}
		}
	}
	levels[1].unlocked = true;
	existed = true;
	return true;
}

bool SaveData::save() const {
	std::string tmp = myPath;
	size_t slash = tmp.find_last_of('/');
	tmp = (slash == std::string::npos ? std::string() : tmp.substr(0, slash + 1)) + "save.tmp";
	{
		std::ofstream f(tmp, std::ios::trunc);
		if (!f) return false;
		f << "leafwind-save 1\n";
		for (int i = 1; i <= NUM_LEVELS; ++i) {
			const LevelRecord& r = levels[i];
			char buf[256];
			std::snprintf(buf, sizeof(buf), "level %d unlocked=%d done=%d bananas=%d total=%d fig=%d deaths=%d time=%.1f\n",
			              i, r.unlocked, r.done, r.bananas, r.total, r.fig, r.deaths, r.time);
			f << buf;
		}
		char buf[64];
		std::snprintf(buf, sizeof(buf), "options volume=%.2f music=%d\n", volume, music ? 1 : 0);
		f << buf;
		if (!f) return false;
	}
	return std::rename(tmp.c_str(), myPath.c_str()) == 0;
}

NewBest SaveData::recordCompletion(int level, const RunResult& r) {
	NewBest nb;
	if (level < 1 || level > NUM_LEVELS) return nb;
	LevelRecord& L = levels[level];
	nb.first = !L.done;
	if (r.bananas > L.bananas || nb.first) {
		nb.bananas = !nb.first && r.bananas > L.bananas;
		L.bananas = std::max(L.bananas, r.bananas);
	}
	if (r.fig && !L.fig) nb.fig = true;
	L.fig = L.fig || r.fig;
	if (nb.first || r.deaths < L.deaths) {
		nb.deaths = !nb.first;
		L.deaths = r.deaths;
	}
	if (nb.first || L.time <= 0.f || r.time < L.time) {
		nb.time = !nb.first;
		L.time = r.time;
	}
	L.total = r.total;
	L.done = true;
	L.unlocked = true;
	if (level < NUM_LEVELS) levels[level + 1].unlocked = true;
	return nb;
}

int SaveData::firstUnfinished() const {
	for (int i = 1; i <= NUM_LEVELS; ++i)
		if (levels[i].unlocked && !levels[i].done) return i;
	return 1;
}

bool SaveData::anyProgress() const {
	for (int i = 1; i <= NUM_LEVELS; ++i)
		if (levels[i].done) return true;
	return false;
}

int SaveData::totalBananas() const {
	int n = 0;
	for (int i = 1; i <= NUM_LEVELS; ++i) n += levels[i].bananas;
	return n;
}

int SaveData::totalFigs() const {
	int n = 0;
	for (int i = 1; i <= NUM_LEVELS; ++i) n += levels[i].fig;
	return n;
}
