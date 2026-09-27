// Leafwind -- entry point and command-line modes (docs/DESIGN.md 8.1).
//
//   monkey_game                              play (title screen)
//   monkey_game --play <level>               skip menus, start a level
//   monkey_game --validate-levels            headless level validation
//   monkey_game --test-physics [level]       headless physics invariants
//   monkey_game --screenshot <what> <frames> <out.png> [--place <tx> <ty>]
//        <what>: a level number 1-10, title, select, credits, card:prologue, card:<N>,
//                card:out<N>, card:epilogue, card:bonus, pause:<N>, results:<N>
//   monkey_game --flow-test                  drive menus/cards/levels with synthetic keys
//   --mute                                   no audio device use at all
#include "audio/Audio.h"
#include "audio/Synth.h"
#include "core/Config.h"
#include "core/Headless.h"
#include "core/Level.h"
#include "core/Save.h"
#include "game/Game.h"
#include "gfx/Assets.h"

#include <SFML/Graphics.hpp>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

static sf::View letterbox(unsigned w, unsigned h) {
	sf::View v(sf::FloatRect(0.f, 0.f, cfg::VIEW_W, cfg::VIEW_H));
	float wr = static_cast<float>(w) / h, vr = cfg::VIEW_W / cfg::VIEW_H;
	float sx = 1.f, sy = 1.f, px = 0.f, py = 0.f;
	if (wr >= vr) {
		sx = vr / wr;
		px = (1.f - sx) * 0.5f;
	} else {
		sy = wr / vr;
		py = (1.f - sy) * 0.5f;
	}
	v.setViewport(sf::FloatRect(px, py, sx, sy));
	return v;
}

static void usage() {
	std::printf("usage: monkey_game [--play N] [--mute] [--validate-levels] [--test-physics [N]]\n"
	            "                   [--screenshot <level|title|select|credits|card:X|pause:N|results:N> <frames> <out.png>\n"
	            "                    [--place <tx> <ty>]]\n");
}

static int runScreenshot(const std::string& what, int frames, const std::string& out, int placeX, int placeY) {
	sf::RenderWindow win(sf::VideoMode(800, 600), "Leafwind", sf::Style::Titlebar | sf::Style::Close);
	Assets assets;
	if (!assets.load()) {
		std::fprintf(stderr, "failed to load assets from %s\n", dataPath("").c_str());
		return 1;
	}
	Audio audio(false);
	SaveData save;
	save.setPath("/nonexistent/leafwind-screenshot-save.txt");
	if (what == "select")
		for (int i = 1; i <= NUM_LEVELS; ++i) save.levels[i].unlocked = true;
	Game game(assets, audio, save, false);
	game.deterministic = true;
	game.setFocused(false);

	bool isLevel = !what.empty() && std::isdigit(static_cast<unsigned char>(what[0]));
	int pauseLevel = 0;
	if (what == "title") game.goTitle();
	else if (what == "select") {
		game.goTitle();
		game.goLevelSelect(1);
	} else if (what == "credits") game.goCredits();
	else if (what.rfind("card:", 0) == 0) game.showCardsNamed(what.substr(5));
	else if (what.rfind("pause:", 0) == 0) {
		pauseLevel = std::atoi(what.c_str() + 6);
		game.startLevel(pauseLevel);
	} else if (what.rfind("results:", 0) == 0) {
		game.showResultsFor(std::atoi(what.c_str() + 8));
	} else if (isLevel) {
		int lvl = std::atoi(what.c_str());
		if (lvl < 1 || lvl > NUM_LEVELS) {
			std::fprintf(stderr, "level must be 1..%d\n", NUM_LEVELS);
			return 1;
		}
		game.startLevel(lvl);
	} else {
		usage();
		return 1;
	}
	if (placeX >= 0 && game.world()) {
		game.world()->placePlayer(placeX * cfg::TILE + 0.25f, (placeY + 1) * cfg::TILE - cfg::PLAYER_H * 0.5f);
		if (game.renderer()) game.renderer()->resetCamera(*game.world());
	}
	for (int i = 0; i < frames; ++i) {
		sf::Event e;
		while (win.pollEvent(e)) {
		}
		game.update(cfg::DT);
	}
	if (pauseLevel) game.openPause();

	sf::RenderTexture shot;
	if (!shot.create(800, 600)) {
		std::fprintf(stderr, "cannot create render texture\n");
		return 1;
	}
	shot.clear(sf::Color::Black);
	game.render(shot);
	shot.display();
	sf::Image img = shot.getTexture().copyToImage();
	win.clear();
	sf::Sprite s(shot.getTexture());
	win.draw(s);
	win.display();
	if (!img.saveToFile(out)) {
		std::fprintf(stderr, "Failed to save screenshot to: %s\n", out.c_str());
		return 1;
	}
	std::printf("Screenshot saved to: %s\n", out.c_str());
	return 0;
}

// Drives the real Game (menus, cards, play, pause, results) with synthetic key presses.
static int runFlowTest() {
	sf::RenderWindow win(sf::VideoMode(800, 600), "Leafwind flow test", sf::Style::Titlebar | sf::Style::Close);
	Assets assets;
	if (!assets.load()) return 1;
	Audio audio(false);
	SaveData save;
	std::string savePath = std::string(std::getenv("TMPDIR") ? std::getenv("TMPDIR") : "/tmp") + "/leafwind-flow-test-save.txt";
	std::remove(savePath.c_str());
	save.setPath(savePath);
	Game game(assets, audio, save, true);
	game.deterministic = true;
	game.setFocused(false);
	sf::RenderTexture frame;
	frame.create(800, 600);
	int fails = 0;
	auto run = [&](int n) {
		for (int i = 0; i < n; ++i) {
			sf::Event e;
			while (win.pollEvent(e)) {
			}
			game.update(cfg::DT);
			if (i % 10 == 0) {
				frame.clear();
				game.render(frame);
				frame.display();
			}
		}
	};
	auto press = [&](sf::Keyboard::Key k, int after = 20) {
		sf::Event e;
		e.type = sf::Event::KeyPressed;
		e.key.code = k;
		e.key.alt = e.key.control = e.key.shift = e.key.system = false;
		game.handleEvent(e);
		run(after);
	};
	auto expect = [&](Screen s, const char* what) {
		bool ok = game.screen() == s;
		std::printf("  %s %s\n", ok ? "ok  " : "FAIL", what);
		if (!ok) ++fails;
	};
	game.goTitle();
	run(60);
	expect(Screen::Title, "title screen");
	press(sf::Keyboard::Space, 60); // New Game (selected without a save)
	expect(Screen::Cards, "prologue card");
	press(sf::Keyboard::Space, 60);
	expect(Screen::Cards, "level 1 intro card");
	press(sf::Keyboard::Space, 60);
	expect(Screen::Playing, "level 1 playing");
	press(sf::Keyboard::Escape, 10);
	press(sf::Keyboard::Down, 5);
	press(sf::Keyboard::Down, 5);
	press(sf::Keyboard::Down, 5);
	press(sf::Keyboard::Space, 60); // Level select
	expect(Screen::LevelSelect, "pause -> level select");
	press(sf::Keyboard::Escape, 60);
	expect(Screen::Title, "level select -> title");
	// complete level 1: start it, put Pip at the exit
	game.goLevelSelect(1);
	run(10);
	press(sf::Keyboard::Space, 60);
	expect(Screen::Cards, "level 1 intro card from select");
	press(sf::Keyboard::Escape, 60);
	expect(Screen::Playing, "cards skipped with Esc");
	World* w = game.world();
	if (w) w->placePlayer(w->exitX * cfg::TILE + 0.25f, (w->exitY + 1) * cfg::TILE - 0.3f);
	for (int i = 0; i < 90 && w && !w->complete; ++i) {
		sf::Event e;
		while (win.pollEvent(e)) {
		}
		game.update(cfg::DT);
	}
	bool completed = w && w->complete;
	std::printf("  %s level completion\n", completed ? "ok  " : "FAIL");
	if (!completed) ++fails;
	run(150);
	expect(Screen::Cards, "outro card");
	press(sf::Keyboard::Space, 60);
	expect(Screen::Results, "results screen");
	press(sf::Keyboard::Space, 60); // Next level
	expect(Screen::Cards, "level 2 intro card");
	press(sf::Keyboard::Space, 60);
	expect(Screen::Playing, "level 2 playing");
	bool unlocked = save.levels[2].unlocked && save.levels[1].done;
	SaveData reread;
	reread.setPath(savePath);
	bool persisted = reread.load() && reread.levels[1].done && reread.levels[2].unlocked;
	std::printf("  %s progress saved and reloaded\n", unlocked && persisted ? "ok  " : "FAIL");
	if (!(unlocked && persisted)) ++fails;
	std::remove(savePath.c_str());
	std::printf(fails ? "flow test FAILED (%d)\n" : "flow test passed\n", fails);
	return fails ? 1 : 0;
}

// Writes every synthesized sound as a 16-bit mono WAV (no audio device needed).
static int runDumpAudio(const std::string& dir) {
	auto sounds = synthesizeAllSounds();
	for (const auto& [name, buf] : sounds) {
		auto pcm = synth::toPCM(buf);
		std::string path = dir + "/" + name + ".wav";
		FILE* f = std::fopen(path.c_str(), "wb");
		if (!f) {
			std::fprintf(stderr, "cannot write %s\n", path.c_str());
			return 1;
		}
		auto u32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
		auto u16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
		uint32_t bytes = static_cast<uint32_t>(pcm.size() * 2);
		std::fwrite("RIFF", 1, 4, f);
		u32(36 + bytes);
		std::fwrite("WAVEfmt ", 1, 8, f);
		u32(16);
		u16(1);
		u16(1);
		u32(synth::SR);
		u32(synth::SR * 2);
		u16(2);
		u16(16);
		std::fwrite("data", 1, 4, f);
		u32(bytes);
		std::fwrite(pcm.data(), 2, pcm.size(), f);
		std::fclose(f);
	}
	std::printf("wrote %zu sounds to %s\n", sounds.size(), dir.c_str());
	return 0;
}

// Renders a level every frame for N frames and reports the average frame cost.
static int runBench(int level, int frames) {
	sf::RenderWindow win(sf::VideoMode(800, 600), "Leafwind bench", sf::Style::Titlebar | sf::Style::Close);
	Assets assets;
	if (!assets.load()) return 1;
	Audio audio(false);
	SaveData save;
	save.setPath("/nonexistent/leafwind-bench.txt");
	Game game(assets, audio, save, false);
	game.deterministic = true;
	game.setFocused(false);
	game.startLevel(level);
	sf::RenderTexture rt;
	rt.create(800, 600);
	sf::Clock clk;
	float upd = 0.f, ren = 0.f;
	for (int i = 0; i < frames; ++i) {
		sf::Event e;
		while (win.pollEvent(e)) {
		}
		clk.restart();
		game.update(cfg::DT);
		upd += clk.restart().asSeconds();
		rt.clear();
		game.render(rt);
		rt.display();
		(void)rt.getTexture().copyToImage().getPixel(0, 0); // force the GPU to finish
		ren += clk.restart().asSeconds();
	}
	std::printf("level %d: update %.2f ms, render %.2f ms per frame (%d frames)\n", level, upd * 1000.f / frames,
	            ren * 1000.f / frames, frames);
	return 0;
}

static int runGame(int playLevel, bool mute) {
	sf::RenderWindow win(sf::VideoMode(800, 600), "Leafwind");
	win.setFramerateLimit(60);
	win.setKeyRepeatEnabled(false);
	Assets assets;
	if (!assets.load()) {
		std::fprintf(stderr, "failed to load assets from %s\n", dataPath("").c_str());
		return 1;
	}
	bool audioOn = !mute && !std::getenv("LEAFWIND_NO_AUDIO");
	Audio audio(audioOn);
	SaveData save;
	save.load();
	Game game(assets, audio, save, true);
	sf::View view = letterbox(800, 600);
	if (playLevel > 0) game.startLevel(playLevel);
	else game.goTitle();

	sf::Clock clk;
	while (win.isOpen()) {
		sf::Event e;
		while (win.pollEvent(e)) {
			if (e.type == sf::Event::Closed) win.close();
			if (e.type == sf::Event::Resized) {
				view = letterbox(e.size.width, e.size.height);
				sf::FloatRect vp = view.getViewport();
				game.setPixelSize(static_cast<unsigned>(vp.width * e.size.width), static_cast<unsigned>(vp.height * e.size.height));
			}
			game.handleEvent(e);
		}
		float dt = std::min(clk.restart().asSeconds(), 0.1f);
		game.update(dt);
		if (game.quitRequested()) win.close();
		win.clear(sf::Color::Black);
		win.setView(view);
		game.render(win);
		win.display();
	}
	save.save();
	return 0;
}

int main(int argc, char** argv) {
	initDataDir(argc > 0 ? argv[0] : nullptr);
	int playLevel = 0;
	bool mute = false;
	std::string shotWhat, shotOut;
	int shotFrames = 0;
	int placeX = -1, placeY = -1;
	for (int i = 1; i < argc; ++i) {
		std::string a = argv[i];
		if (a == "--validate-levels") return runValidateLevels();
		if (a == "--flow-test") return runFlowTest();
		if (a == "--dump-audio" && i + 1 < argc) return runDumpAudio(argv[i + 1]);
		if (a == "--bench" && i + 2 < argc) return runBench(std::atoi(argv[i + 1]), std::atoi(argv[i + 2]));
		if (a == "--test-physics") {
			int lvl = (i + 1 < argc && std::isdigit(static_cast<unsigned char>(argv[i + 1][0]))) ? std::atoi(argv[i + 1]) : 0;
			return runPhysicsTest(lvl);
		}
		if (a == "--screenshot" && i + 3 < argc) {
			shotWhat = argv[i + 1];
			shotFrames = std::atoi(argv[i + 2]);
			shotOut = argv[i + 3];
			i += 3;
		} else if (a == "--place" && i + 2 < argc) {
			placeX = std::atoi(argv[i + 1]);
			placeY = std::atoi(argv[i + 2]);
			i += 2;
		} else if (a == "--play" && i + 1 < argc) {
			playLevel = std::atoi(argv[++i]);
			if (playLevel < 1 || playLevel > NUM_LEVELS) {
				std::fprintf(stderr, "level must be 1..%d\n", NUM_LEVELS);
				return 1;
			}
		} else if (a == "--mute") {
			mute = true;
		} else if (a == "--help" || a == "-h") {
			usage();
			return 0;
		} else {
			std::fprintf(stderr, "unknown argument: %s\n", a.c_str());
			usage();
			return 1;
		}
	}
	if (!shotWhat.empty()) return runScreenshot(shotWhat, shotFrames, shotOut, placeX, placeY);
	return runGame(playLevel, mute);
}
