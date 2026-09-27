// Leafwind -- screens and flow: title, level select, story cards, play (+pause), results,
// credits (docs/DESIGN.md section 7).
#pragma once

#include "InputDevice.h"
#include "audio/Audio.h"
#include "core/Save.h"
#include "core/World.h"
#include "gfx/Assets.h"
#include "gfx/Renderer.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

enum class Screen { Title, LevelSelect, Cards, Playing, Results, Credits };

struct CardPage {
	std::vector<std::string> lines;
	int art = 0; // 0 leaf, 1 wind swirl, 2 golden leaf, 3 figs, 4 Pip asleep on the leaf
	Biome biome = Biome::Canopy;
};

class Game {
public:
	Game(const Assets& assets, Audio& audio, SaveData& save, bool allowSave);

	void setPixelSize(unsigned w, unsigned h);
	void handleEvent(const sf::Event& e);
	void update(float dt);
	void render(sf::RenderTarget& t);
	bool quitRequested() const { return myQuit; }

	// Flow entry points
	void goTitle();
	void goLevelSelect(int cursor);
	void newGame();
	void playLevel(int index, bool withIntroCard);
	void startLevel(int index);
	void goCredits();

	// Screenshot / test helpers
	void showCardsNamed(const std::string& which); // "prologue", "N", "outN", "epilogue", "bonus"
	void openPause() { paused = true; }
	void showResultsFor(int index);
	World* world() { return myWorld.get(); }
	Renderer* renderer() { return myRenderer.get(); }
	Screen screen() const { return myScreen; }
	void setFocused(bool f) { focused = f; }
	bool deterministic = false; // fixed time, no real clocks

private:
	void loadWorld(int index);
	void showCards(std::vector<CardPage> pages, std::function<void()> then);
	void transition(std::function<void()> fn, float outTime = 0.3f);
	void onLevelComplete();
	void finishLevelFlow();
	void showResults();
	std::vector<CardPage> levelCards(int index, bool outro) const;

	void updateTitle(float dt, const MenuInput& m);
	void updateSelect(float dt, const MenuInput& m);
	void updateCards(float dt, const MenuInput& m);
	void updatePlaying(float dt, const MenuInput& m);
	void updateResults(float dt, const MenuInput& m);
	void updateCredits(float dt, const MenuInput& m);

	void drawScene(sf::RenderTarget& t, float desat);
	void drawTitle(sf::RenderTarget& t);
	void drawSelect(sf::RenderTarget& t);
	void drawCards(sf::RenderTarget& t);
	void drawHud(sf::RenderTarget& t);
	void drawPause(sf::RenderTarget& t);
	void drawResults(sf::RenderTarget& t);
	void drawCredits(sf::RenderTarget& t);
	void drawSignBubble(sf::RenderTarget& t);
	void drawMenu(sf::RenderTarget& t, const std::vector<std::string>& items, int sel, V2 pos, unsigned size,
	              sf::Color col, sf::Color hi);

	const Assets& assets;
	Audio& audio;
	SaveData& save;
	bool allowSave;
	bool myQuit = false;
	bool focused = true;
	InputDevice input;

	Screen myScreen = Screen::Title;
	float clock = 0.f;      // screen-independent animation clock
	float volumeShown = 0.f;
	float screenT = 0.f;    // time on the current screen

	// fading
	float fade = 1.f;       // 0 = clear, 1 = black
	float fadeDir = -1.f;   // -1 fading in, +1 fading out
	float fadeSpeed = 1.f / 0.4f;
	std::function<void()> pending;

	// level / world
	int levelIndex = 1;
	std::unique_ptr<LevelData> myLevel;
	std::unique_ptr<World> myWorld;
	std::unique_ptr<Renderer> myRenderer;
	sf::RenderTexture scene;
	bool sceneOk = false;
	float acc = 0.f;
	float levelT = 0.f;
	float hudAlpha = 1.f, hudIdle = 0.f, hudPop = 0.f;
	float completeT = -1.f;
	float respawnFade = 0.f;
	bool paused = false;
	int pauseSel = 0;
	bool campaign = false;  // came from New Game / Next (story flow)

	// menus
	int titleSel = 0;
	int selectCursor = 0;
	int resultsSel = 0;
	RunResult lastRun;
	NewBest lastBest;

	// cards
	std::vector<CardPage> cards;
	size_t cardIdx = 0;
	float cardT = 0.f;
	float cardOut = -1.f;
	std::function<void()> cardsThen;

	// floating world-space labels ("Checkpoint!", "Golden fig!")
	struct Popup {
		b2Vec2 at; // metres
		float t = 0.f;
		std::string text;
		sf::Color col;
	};
	std::vector<Popup> popups;
	void drawPopups(sf::RenderTarget& t);

	// screen-space drifting leaves (title, credits)
	Particles screenLeaves;
	void spawnScreenLeaves(float dt, float perSecond, float sizeScale);
};
