// Leafwind -- entry point and command-line modes (docs/DESIGN.md 8.1).
//
//   monkey_game                              play (title screen)
//   monkey_game --play <level>               skip menus, start a level
//   monkey_game --validate-levels            headless level validation
//   monkey_game --test-physics [level]       headless physics invariants
//   monkey_game --screenshot <what> <frames> <out.png> [--place <tx> <ty>]
//        <what>: a level number 1-10, title, select, credits, card:prologue, card:<N>,
//                card:out<N>, card:epilogue, card:bonus, pause:<N>, results:<N>
//   --mute                                   no audio device use at all
#include "audio/Audio.h"
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
