#include "Game.h"

#include "UI.h"
#include "core/Config.h"

#include <algorithm>
#include <cmath>

using namespace cfg;
using namespace uidraw;

namespace {
const float PI = 3.14159265f;
const sf::Color PAPER(0xFF, 0xF4, 0xD6);

std::string upper(std::string s) {
	for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
	return s;
}

void dim(sf::RenderTarget& t, int alpha) {
	Canvas c;
	c.rect(0, 0, VIEW_W, VIEW_H, sf::Color(0, 0, 0, static_cast<sf::Uint8>(alpha)));
	c.draw(t);
}

Biome biomeOfLevel(int idx) {
	static const Biome b[] = {Biome::Canopy, Biome::Canopy, Biome::Grove, Biome::Gully, Biome::River, Biome::Hollow,
	                          Biome::Ridge,  Biome::Cliffs, Biome::Swamp, Biome::Mill,  Biome::Storm};
	return b[std::clamp(idx, 0, 10)];
}

std::string levelName(int idx) {
	static const char* n[] = {"", "Home Tree", "Vine Grove", "The Gully", "Riverbend", "Mushroom Hollow",
	                          "Boulder Ridge", "Windy Cliffs", "Sighing Swamp", "The Old Mill", "The Storm Tree"};
	return n[std::clamp(idx, 0, 10)];
}
} // namespace

Game::Game(const Assets& a, Audio& au, SaveData& s, bool allow) : assets(a), audio(au), save(s), allowSave(allow) {
	setPixelSize(static_cast<unsigned>(VIEW_W), static_cast<unsigned>(VIEW_H));
	// banana totals for the level-select cards (also filled for levels never played)
	for (int i = 1; i <= NUM_LEVELS; ++i) {
		LevelData L;
		std::string err;
		if (loadLevelIndex(i, L, err)) save.levels[i].total = L.count('b');
	}
	creditLeaves.seed(99);
	audio.setVolume(save.volume);
	audio.setMusicEnabled(save.music);
}

void Game::setPixelSize(unsigned w, unsigned h) {
	w = std::max(1u, w);
	h = std::max(1u, h);
	if (sceneOk && scene.getSize().x == w && scene.getSize().y == h) return;
	sceneOk = scene.create(w, h);
	scene.setSmooth(true);
}

void Game::handleEvent(const sf::Event& e) {
	input.handleEvent(e);
	if (e.type == sf::Event::LostFocus) {
		focused = false;
		if (myScreen == Screen::Playing && completeT < 0.f) paused = true;
	}
	if (e.type == sf::Event::GainedFocus) focused = true;
}

// ---------------------------------------------------------------------------
// Flow

void Game::loadWorld(int index) {
	myRenderer.reset();
	myWorld.reset();
	myLevel = std::make_unique<LevelData>();
	std::string err;
	if (!loadLevelIndex(index, *myLevel, err)) {
		// fall back to level 1 so the game never ends up without a world
		loadLevelIndex(1, *myLevel, err);
		index = 1;
	}
	levelIndex = index;
	myWorld = std::make_unique<World>(*myLevel);
	myRenderer = std::make_unique<Renderer>(assets);
	myRenderer->setLevel(*myLevel, index);
	myRenderer->resetCamera(*myWorld);
	audio.setBiome(myLevel->biome);
}

void Game::transition(std::function<void()> fn, float outTime) {
	pending = std::move(fn);
	fadeDir = 1.f;
	fadeSpeed = 1.f / std::max(0.05f, outTime);
}

void Game::goTitle() {
	loadWorld(1);
	myRenderer->hidePlayer = true;
	myScreen = Screen::Title;
	screenT = 0.f;
	titleSel = save.anyProgress() ? 0 : 1;
	paused = false;
	fadeDir = -1.f;
	fadeSpeed = 1.f / 0.6f;
}

void Game::goLevelSelect(int cursor) {
	if (!myWorld || myLevel->index != 1 || myScreen == Screen::Playing || myScreen == Screen::Results) {
		loadWorld(1);
		myRenderer->hidePlayer = true;
	}
	myScreen = Screen::LevelSelect;
	screenT = 0.f;
	selectCursor = std::clamp(cursor - 1, 0, NUM_LEVELS - 1);
	paused = false;
	fadeDir = -1.f;
}

void Game::newGame() {
	campaign = true;
	std::vector<CardPage> pro = {{{"Pip is the smallest monkey in the Home Tree,",
	                               "and the only one who has never swung on a vine.", "\"Heights,\" says Pip, \"are for birds.\""},
	                              0,
	                              Biome::Canopy}};
	auto intro = levelCards(1, false);
	pro.insert(pro.end(), intro.begin(), intro.end());
	showCards(pro, [this]() { startLevel(1); });
}

void Game::playLevel(int index, bool withIntroCard) {
	if (withIntroCard) showCards(levelCards(index, false), [this, index]() { startLevel(index); });
	else startLevel(index);
}

void Game::startLevel(int index) {
	loadWorld(index);
	myScreen = Screen::Playing;
	screenT = 0.f;
	levelT = 0.f;
	acc = 0.f;
	hudAlpha = 1.f;
	hudIdle = 0.f;
	completeT = -1.f;
	paused = false;
	pauseSel = 0;
	respawnFade = 0.f;
	fade = 1.f;
	fadeDir = -1.f;
	fadeSpeed = 1.f / 0.4f;
	input.clear();
}

std::vector<CardPage> Game::levelCards(int index, bool outro) const {
	LevelData L;
	std::string err;
	std::vector<CardPage> out;
	if (!loadLevelIndex(index, L, err)) return out;
	CardPage p;
	p.lines = outro ? L.cardOut : L.cardIn;
	p.biome = L.biome;
	std::string all;
	for (auto& s : p.lines) all += s + " ";
	p.art = 0;
	if (all.find("Wind") != std::string::npos) p.art = 1;
	if (index == 10 && outro) p.art = 2;
	if (index == 10 && !outro) p.art = 2;
	if (!p.lines.empty()) out.push_back(p);
	return out;
}

void Game::showCards(std::vector<CardPage> pages, std::function<void()> then) {
	if (pages.empty()) {
		then();
		return;
	}
	cards = std::move(pages);
	cardIdx = 0;
	cardT = 0.f;
	cardOut = -1.f;
	cardsThen = std::move(then);
	myScreen = Screen::Cards;
	screenT = 0.f;
	fade = 0.f;
	fadeDir = -1.f;
	audio.setBiome(cards[0].biome);
}

void Game::showCardsNamed(const std::string& which) {
	if (which == "prologue") {
		newGame();
		return;
	}
	if (which == "epilogue" || which == "bonus") {
		std::vector<CardPage> ep = {
		    {{"Pip swung home. Not the long way round.", "Straight down the Cliffs, laughing.",
		      "Nana Fig was waiting, with two cups of fig tea."},
		     0,
		     Biome::Canopy},
		    {{"That night Pip slept on the big green leaf,", "which smelled of home,", "and a little of mountain."},
		     4,
		     Biome::Canopy},
		};
		char buf[96];
		std::snprintf(buf, sizeof(buf), "(Bananas: %d / 262 \xC2\xB7 Golden figs: %d / 10)", save.totalBananas(), save.totalFigs());
		ep.push_back({{"THE END", buf}, 2, Biome::Canopy});
		if (which == "bonus" || save.totalFigs() >= 10)
			ep.push_back({{"Pip brought Nana Fig ten golden figs.", "She ate every one,", "and then she asked for more."},
			              3,
			              Biome::Canopy});
		if (which == "bonus") ep.erase(ep.begin(), ep.begin() + 3);
		showCards(ep, [this]() { goCredits(); });
		return;
	}
	bool outro = which.rfind("out", 0) == 0;
	int idx = std::atoi(which.c_str() + (outro ? 3 : 0));
	if (idx < 1 || idx > NUM_LEVELS) idx = 1;
	if (outro) showCards(levelCards(idx, true), [this, idx]() { showResultsFor(idx); });
	else showCards(levelCards(idx, false), [this, idx]() { startLevel(idx); });
}

void Game::onLevelComplete() {
	lastRun.bananas = myWorld->bananas;
	lastRun.total = myWorld->bananasTotal;
	lastRun.deaths = myWorld->deaths;
	lastRun.fig = myWorld->fig;
	lastRun.time = myWorld->levelTimer;
	lastBest = save.recordCompletion(levelIndex, lastRun);
	if (allowSave) save.save();
}

void Game::finishLevelFlow() {
	int idx = levelIndex;
	auto outro = levelCards(idx, true);
	if (idx == NUM_LEVELS) {
		showCards(outro, [this]() { showCardsNamed("epilogue"); });
	} else {
		showCards(outro, [this]() { showResults(); });
	}
}

void Game::showResultsFor(int index) {
	if (!myWorld || levelIndex != index) loadWorld(index);
	myRenderer->hidePlayer = true;
	lastRun.total = myWorld->bananasTotal;
	showResults();
}

void Game::showResults() {
	myScreen = Screen::Results;
	screenT = 0.f;
	resultsSel = 0;
	fadeDir = -1.f;
	if (myRenderer) myRenderer->hidePlayer = true;
}

void Game::goCredits() {
	myScreen = Screen::Credits;
	screenT = 0.f;
	creditLeaves.clear();
	audio.setBiome(Biome::Canopy);
	fadeDir = -1.f;
}

// ---------------------------------------------------------------------------
// Update

void Game::update(float dt) {
	clock += dt;
	screenT += dt;
	if (fadeDir > 0.f) {
		fade = std::min(1.f, fade + dt * fadeSpeed);
		if (fade >= 1.f) {
			auto fn = std::move(pending);
			pending = nullptr;
			fadeDir = -1.f;
			fadeSpeed = 1.f / 0.4f;
			if (fn) fn();
		}
	} else {
		fade = std::max(0.f, fade - dt * fadeSpeed);
	}
	MenuInput m = input.menu();
	if (fadeDir > 0.f) m = MenuInput(); // ignore input while fading out
	switch (myScreen) {
	case Screen::Title: updateTitle(dt, m); break;
	case Screen::LevelSelect: updateSelect(dt, m); break;
	case Screen::Cards: updateCards(dt, m); break;
	case Screen::Playing: updatePlaying(dt, m); break;
	case Screen::Results: updateResults(dt, m); break;
	case Screen::Credits: updateCredits(dt, m); break;
	}
	float musicLevel = 1.f;
	if (myScreen == Screen::Cards) musicLevel = 0.f;
	if (myScreen == Screen::Results) musicLevel = 0.6f;
	if (paused) musicLevel = 0.4f;
	audio.update(myScreen == Screen::Playing && myWorld ? myWorld.get() : nullptr, dt, musicLevel);
}

static int menuMove(const MenuInput& m, int sel, int n, Audio& audio) {
	int s = sel;
	if (m.up) s = (s + n - 1) % n;
	if (m.down) s = (s + 1) % n;
	if (s != sel) audio.play(Sfx::UiMove);
	return s;
}

void Game::updateTitle(float dt, const MenuInput& m) {
	if (myWorld && myRenderer) {
		myRenderer->update(*myWorld, dt, clock);
		myWorld->events.clear();
		float W = myLevel->width * TILE_PX;
		float u = 0.5f - 0.5f * std::cos(clock * 0.06f);
		myRenderer->cam.setCenter(V2(400.f + u * (W - 800.f), myLevel->height * TILE_PX - 300.f));
	}
	bool hasContinue = save.anyProgress();
	titleSel = menuMove(m, titleSel, 4, audio);
	if (!hasContinue && titleSel == 0) titleSel = m.up ? 3 : 1;
	if (m.confirm) {
		audio.play(Sfx::UiConfirm);
		switch (titleSel) {
		case 0: transition([this]() { goLevelSelect(save.firstUnfinished()); }); break;
		case 1: transition([this]() { newGame(); }); break;
		case 2: transition([this]() { goLevelSelect(save.firstUnfinished()); }); break;
		case 3: myQuit = true; break;
		}
	}
	if (m.back && screenT > 0.5f) myQuit = true;
}

void Game::updateSelect(float dt, const MenuInput& m) {
	if (myWorld && myRenderer) {
		myRenderer->update(*myWorld, dt, clock);
		myWorld->events.clear();
		float W = myLevel->width * TILE_PX;
		float u = 0.5f - 0.5f * std::cos(clock * 0.06f);
		myRenderer->cam.setCenter(V2(400.f + u * (W - 800.f), myLevel->height * TILE_PX - 300.f));
	}
	int c = selectCursor;
	if (m.left) c = (c + NUM_LEVELS - 1) % NUM_LEVELS;
	if (m.right) c = (c + 1) % NUM_LEVELS;
	if (m.up || m.down) c = (c + 5) % NUM_LEVELS;
	if (c != selectCursor) {
		selectCursor = c;
		audio.play(Sfx::UiMove);
	}
	if (m.confirm) {
		int idx = selectCursor + 1;
		if (save.levels[idx].unlocked) {
			audio.play(Sfx::UiConfirm);
			campaign = false;
			transition([this, idx]() { playLevel(idx, true); });
		}
	}
	if (m.back) transition([this]() { goTitle(); });
}

void Game::updateCards(float dt, const MenuInput& m) {
	cardT += dt;
	if (m.back) {
		auto fn = cardsThen;
		transition([fn]() {
			if (fn) fn();
		}, 0.25f);
		return;
	}
	if (cardOut >= 0.f) {
		cardOut += dt;
		if (cardOut >= 0.3f) {
			if (cardIdx + 1 < cards.size()) {
				++cardIdx;
				cardT = 0.f;
				cardOut = -1.f;
			} else {
				cardOut = -1.f;
				auto fn = cardsThen;
				fade = 1.f;
				if (fn) fn();
			}
		}
		return;
	}
	if (m.confirm && cardT > 0.35f) {
		audio.play(Sfx::UiMove);
		cardOut = 0.f;
	}
}

void Game::updatePlaying(float dt, const MenuInput& m) {
	if (!myWorld) return;
	World& w = *myWorld;
	if (paused) {
		pauseSel = menuMove(m, pauseSel, 5, audio);
		if (m.pause || m.back) {
			paused = false;
			input.clear();
			return;
		}
		if (m.confirm) {
			audio.play(Sfx::UiConfirm);
			switch (pauseSel) {
			case 0: paused = false; break;
			case 1: {
				paused = false;
				Input in;
				in.restartPressed = true;
				w.step(in);
				w.events.clear();
				myRenderer->resetCamera(w);
				respawnFade = 0.2f;
				break;
			}
			case 2: {
				int idx = levelIndex;
				transition([this, idx]() { startLevel(idx); });
				break;
			}
			case 3: {
				int idx = levelIndex;
				transition([this, idx]() { goLevelSelect(idx); });
				break;
			}
			case 4: transition([this]() { goTitle(); }); break;
			}
			input.clear();
		}
		return;
	}
	if (m.pause && completeT < 0.f && fadeDir <= 0.f) {
		paused = true;
		pauseSel = 0;
		audio.play(Sfx::UiMove);
		return;
	}
	levelT += dt;
	respawnFade = std::max(0.f, respawnFade - dt);
	acc += dt;
	int steps = 0;
	while (acc >= DT - 1e-6f && steps < MAX_STEPS_PER_FRAME) {
		Input in = input.poll(focused && fadeDir <= 0.f);
		bool wasDead = !w.player.alive();
		int deathsBefore = w.deaths;
		w.step(in);
		if (wasDead && w.player.alive()) {
			respawnFade = 0.2f;
			myRenderer->resetCamera(w);
		}
		if (w.deaths != deathsBefore && w.player.alive()) myRenderer->resetCamera(w); // R restart
		myRenderer->update(w, DT, clock);
		audio.handleEvents(w.events);
		for (const GameEvent& e : w.events) {
			if (e.type == Ev::Banana || e.type == Ev::Fig || e.type == Ev::Death || e.type == Ev::Checkpoint ||
			    e.type == Ev::Respawn)
				hudIdle = 0.f;
			if (e.type == Ev::LevelComplete) {
				completeT = 0.f;
				onLevelComplete();
			}
		}
		w.events.clear();
		if (myRenderer->thunder) {
			audio.play(Sfx::Thunder, 0.9f);
			myRenderer->thunder = false;
		}
		acc -= DT;
		++steps;
	}
	if (steps == MAX_STEPS_PER_FRAME) acc = std::min(acc, DT);
	// HUD auto-hide after 3 s of calm (always shown in the first seconds)
	hudIdle += dt;
	float want = (hudIdle < 3.f || levelT < 3.f) ? 1.f : 0.f;
	hudAlpha += (want - hudAlpha) * std::min(1.f, dt * 4.f);
	if (completeT >= 0.f) {
		completeT += dt;
		if (completeT > 1.3f && fadeDir <= 0.f && !pending) transition([this]() { finishLevelFlow(); }, 0.5f);
	}
}

void Game::updateResults(float dt, const MenuInput& m) {
	if (myWorld && myRenderer) {
		myRenderer->update(*myWorld, dt, clock);
		myWorld->events.clear();
	}
	resultsSel = menuMove(m, resultsSel, 3, audio);
	if (m.confirm) {
		audio.play(Sfx::UiConfirm);
		int idx = levelIndex;
		switch (resultsSel) {
		case 0:
			if (idx < NUM_LEVELS) transition([this, idx]() { playLevel(idx + 1, true); });
			else transition([this]() { goCredits(); });
			break;
		case 1: transition([this, idx]() { startLevel(idx); }); break;
		case 2: transition([this, idx]() { goLevelSelect(idx); }); break;
		}
	}
	if (m.back) {
		int idx = levelIndex;
		transition([this, idx]() { goLevelSelect(idx); });
	}
}

void Game::updateCredits(float dt, const MenuInput& m) {
	// drifting leaves
	if (creditLeaves.rnd(0.f, 1.f) < dt * 6.f) {
		Particle& q = creditLeaves.spawn();
		q.kind = PKind::Leaf;
		q.pos = V2(creditLeaves.rnd(-50.f, 850.f), -20.f);
		q.vel = V2(creditLeaves.rnd(10.f, 40.f), creditLeaves.rnd(30.f, 60.f));
		q.wobble = 30.f;
		q.phase = creditLeaves.rnd(0.f, 6.f);
		q.spin = creditLeaves.rnd(-2.f, 2.f);
		q.maxLife = 16.f;
		q.size = creditLeaves.rnd(6.f, 10.f);
		q.color = creditLeaves.rnd(0.f, 1.f) < 0.8f ? sf::Color(0x6D, 0xBE, 0x45) : ui::GoldCore;
		q.shrink = false;
		q.fadeIn = true;
	}
	creditLeaves.update(dt);
	if ((m.confirm || m.back) && screenT > 1.f) transition([this]() { goTitle(); }, 0.6f);
	if (screenT > 38.f && fadeDir <= 0.f && !pending) transition([this]() { goTitle(); }, 1.f);
}

// ---------------------------------------------------------------------------
// Render

void Game::drawScene(sf::RenderTarget& t, float desat) {
	if (!myWorld || !myRenderer || !sceneOk) return;
	myRenderer->drawWorld(scene, *myWorld, clock);
	scene.display();
	sf::Sprite s(scene.getTexture());
	s.setScale(VIEW_W / scene.getSize().x, VIEW_H / scene.getSize().y);
	if (desat > 0.f && assets.shadersOk) {
		sf::Shader& sh = const_cast<sf::Shader&>(assets.grey);
		sh.setUniform("amount", desat);
		t.draw(s, &sh);
	} else {
		t.draw(s);
		if (desat > 0.f) dim(t, static_cast<int>(120 * desat));
	}
}

void Game::render(sf::RenderTarget& t) {
	sf::View ui(sf::FloatRect(0.f, 0.f, VIEW_W, VIEW_H));
	float deathFade = 0.f;
	switch (myScreen) {
	case Screen::Title:
		drawScene(t, 0.f);
		t.setView(ui);
		drawTitle(t);
		break;
	case Screen::LevelSelect:
		drawScene(t, 0.f);
		t.setView(ui);
		drawSelect(t);
		break;
	case Screen::Cards:
		t.setView(ui);
		drawCards(t);
		break;
	case Screen::Playing: {
		float desat = 0.f;
		if (myWorld && !myWorld->player.alive()) {
			float dtm = myWorld->player.deadTimer;
			desat = std::min(1.f, dtm / 0.3f);
			if (dtm > DEATH_FADE_START) deathFade = std::min(1.f, (dtm - DEATH_FADE_START) / 0.2f);
		}
		drawScene(t, desat);
		t.setView(ui);
		drawSignBubble(t);
		drawHud(t);
		// level title ribbon: slides in from the left for 1.8 s
		if (levelT < 2.4f && myLevel) {
			float u = levelT < 0.35f ? levelT / 0.35f : (levelT > 1.8f ? 1.f - (levelT - 1.8f) / 0.5f : 1.f);
			u = std::clamp(u, 0.f, 1.f);
			float ease = 1.f - (1.f - u) * (1.f - u);
			char buf[128];
			std::snprintf(buf, sizeof(buf), "LEVEL %d \xE2\x80\x94 %s", levelIndex, upper(myLevel->name).c_str());
			float tw = textWidth(assets.bold, buf, 24);
			float w = tw + 90.f;
			float x = -w + ease * (w + 0.f) - 10.f + (800.f - w) * 0.5f * ease;
			Canvas c;
			c.rect(x + 4.f, 214.f, w, 54.f, sf::Color(0, 0, 0, static_cast<sf::Uint8>(50 * u)));
			c.rect(x, 210.f, w, 54.f, withAlpha(ui::Cream, static_cast<int>(235 * u)));
			c.tri(V2(x + w, 210.f), V2(x + w + 18.f, 237.f), V2(x + w, 264.f), withAlpha(ui::Cream, static_cast<int>(235 * u)));
			c.rect(x + 45.f, 250.f, tw, 4.f, withAlpha(paletteFor(myLevel->biome).accent, static_cast<int>(255 * u)));
			c.leaf(V2(x + 18.f, 237.f), V2(1.f, -0.3f), 20.f, 7.f, withAlpha(paletteFor(myLevel->biome).grass, static_cast<int>(255 * u)));
			c.draw(t);
			text(t, assets.bold, buf, 24, V2(x + 45.f, 218.f), ui::Ink, Left, u);
		}
		if (completeT >= 0.f) {
			float u = std::min(1.f, completeT / 0.4f);
			textShadow(t, assets.bold, "Well done, Pip!", 40, V2(400.f, 180.f), ui::Cream, sf::Color(0, 0, 0, 120), V2(2.f, 3.f),
			           Center, u);
		}
		if (paused) drawPause(t);
		break;
	}
	case Screen::Results:
		drawScene(t, 0.f);
		t.setView(ui);
		drawResults(t);
		break;
	case Screen::Credits:
		t.setView(ui);
		drawCredits(t);
		break;
	}
	t.setView(ui);
	float black = std::max({fade, deathFade, respawnFade / 0.2f});
	if (black > 0.f) dim(t, static_cast<int>(255 * std::min(1.f, black)));
}

void Game::drawMenu(sf::RenderTarget& t, const std::vector<std::string>& items, int sel, V2 pos, unsigned size,
                    sf::Color col, sf::Color hi) {
	for (size_t i = 0; i < items.size(); ++i) {
		if (items[i].empty()) continue;
		V2 p = pos + V2(0.f, i * (size + 16.f));
		bool s = static_cast<int>(i) == sel;
		if (s) {
			Canvas c;
			float bob = std::sin(clock * 5.f) * 2.f;
			c.leaf(p + V2(-30.f + bob, size * 0.62f), V2(1.f, -0.25f), 20.f, 7.f, sf::Color(0x6D, 0xBE, 0x45),
			       sf::Color(0x3F, 0x6B, 0x2A));
			c.draw(t);
		}
		textShadow(t, assets.bold, items[i], size, p, s ? hi : col, sf::Color(0, 0, 0, 110), V2(2.f, 2.f));
	}
}

void Game::drawTitle(sf::RenderTarget& t) {
	Canvas c;
	// soft gradient behind the title for legibility
	c.rectV(0, 0, 800, 230, sf::Color(0x1A, 0x12, 0x08, 90), sf::Color(0x1A, 0x12, 0x08, 0));
	// Pip on Nana Fig's big green leaf (bottom-left), gently bobbing
	float bob = std::sin(clock * 1.6f) * 5.f;
	V2 leafC(215.f, 468.f + bob);
	c.ellipse(leafC + V2(10.f, 34.f - bob), 170.f, 16.f, sf::Color(0, 0, 0, 50));
	goldenLeaf(c, leafC, 150.f, 330.f, 1.35f, sf::Color(0x58, 0xB3, 0x48), sf::Color(0x3F, 0x8E, 0x3A));
	goldenLeaf(c, leafC + V2(-4.f, -3.f), 120.f, 300.f, 1.35f, sf::Color(0x6D, 0xBE, 0x45), sf::Color(0x4F, 0x9A, 0x3E));
	c.draw(t);
	sf::Sprite pip(assets.monkey(PAnim::Stand, true));
	pip.setScale(2.f, 2.f);
	pip.setOrigin(32.f, 64.f);
	pip.setPosition(std::round(leafC.x + 5.f), std::round(leafC.y - 6.f));
	t.draw(pip);

	// title
	const std::string title = "LEAFWIND";
	sf::Text tt(title, assets.bold, 64);
	sf::FloatRect b = tt.getLocalBounds();
	V2 tp(400.f - b.width * 0.5f - b.left, 40.f);
	tt.setPosition(tp + V2(3.f, 5.f));
	tt.setFillColor(sf::Color(0x2E, 0x24, 0x18, 170));
	t.draw(tt);
	tt.setPosition(tp);
	tt.setFillColor(ui::Cream);
	tt.setOutlineColor(sf::Color(0x4E, 0x33, 0x20));
	tt.setOutlineThickness(2.f);
	t.draw(tt);
	// a leaf tucked into the F
	V2 fpos = tt.findCharacterPos(3);
	Canvas lf;
	lf.leaf(fpos + V2(28.f, 20.f), norm(V2(1.f, -0.8f)), 30.f, 9.f, sf::Color(0x6D, 0xBE, 0x45), sf::Color(0x3F, 0x6B, 0x2A));
	lf.draw(t);
	textShadow(t, assets.regular, "A little monkey. A big wind. One leaf.", 20, V2(400.f, 130.f), ui::Cream,
	           sf::Color(0, 0, 0, 120), V2(1.f, 2.f), Center);

	// menu
	std::vector<std::string> items = {save.anyProgress() ? "Continue" : "", "New Game", "Level Select", "Quit"};
	Canvas panel;
	panel.roundRect(470.f, 272.f, 250.f, 210.f, 16.f, sf::Color(0x2E, 0x24, 0x18, 110));
	panel.draw(t);
	drawMenu(t, items, titleSel, V2(520.f, 290.f), 28, ui::Cream, ui::Banana);
	text(t, assets.regular, "Arrows / WASD to choose \xC2\xB7 Space to start", 16, V2(400.f, 566.f),
	     withAlpha(ui::Cream, 220), Center);
}

void Game::drawSelect(sf::RenderTarget& t) {
	dim(t, 90);
	textShadow(t, assets.bold, "Choose a level", 40, V2(400.f, 26.f), ui::Cream, sf::Color(0, 0, 0, 140), V2(2.f, 3.f), Center);
	const float cw = 140.f, ch = 205.f, gap = 12.f;
	const float x0 = (800.f - (5 * cw + 4 * gap)) * 0.5f;
	for (int i = 0; i < NUM_LEVELS; ++i) {
		int idx = i + 1;
		const LevelRecord& rec = save.levels[idx];
		bool sel = i == selectCursor;
		float x = x0 + (i % 5) * (cw + gap);
		float y = 100.f + (i / 5) * (ch + 22.f) - (sel ? 6.f : 0.f);
		const Palette& p = paletteFor(biomeOfLevel(idx));
		Canvas c;
		c.roundRect(x + 3.f, y + 5.f, cw, ch, 12.f, sf::Color(0, 0, 0, 70));
		if (sel) c.roundRect(x - 4.f, y - 4.f, cw + 8.f, ch + 8.f, 15.f, ui::Banana);
		c.roundRect(x, y, cw, ch, 12.f, rec.unlocked ? PAPER : sf::Color(0xC8, 0xBE, 0xA4));
		// biome swatch thumbnail
		float sx = x + 10.f, sy = y + 34.f, sw = cw - 20.f;
		c.rectV(sx, sy, sw, 30.f, p.skyTop, p.skyBot);
		for (int k = 0; k < 5; ++k) c.circle(V2(sx + 10.f + k * 26.f, sy + 32.f), 12.f + (k % 2) * 5.f, p.mid, 14);
		c.rect(sx, sy + 30.f, sw, 12.f, p.mid);
		c.rect(sx, sy + 42.f, sw, 5.f, p.grass);
		c.rect(sx, sy + 47.f, sw, 17.f, p.earth);
		c.rect(sx, sy + 58.f, sw, 6.f, p.earthDark);
		c.rect(sx, sy + 50.f, 12.f, 3.f, withAlpha(p.accent, 255));
		if (rec.unlocked) {
			bananaIcon(c, V2(x + 22.f, y + 150.f), 0.9f);
			figIcon(c, V2(x + cw - 22.f, y + 150.f), 0.9f, rec.fig);
		}
		c.draw(t);
		char num[8];
		std::snprintf(num, sizeof(num), "%d", idx);
		text(t, assets.bold, num, 22, V2(x + 12.f, y + 5.f), ui::Ink);
		if (rec.done) {
			Canvas tick;
			tick.line(V2(x + cw - 30.f, y + 18.f), V2(x + cw - 24.f, y + 24.f), 4.f, sf::Color(0x4F, 0x9A, 0x3C));
			tick.line(V2(x + cw - 24.f, y + 24.f), V2(x + cw - 12.f, y + 10.f), 4.f, sf::Color(0x4F, 0x9A, 0x3C));
			tick.draw(t);
		}
		auto nameLines = wrap(assets.bold, levelName(idx), 15, cw - 16.f);
		for (size_t k = 0; k < nameLines.size(); ++k)
			text(t, assets.bold, nameLines[k], 15, V2(x + cw * 0.5f, y + 104.f + k * 18.f), ui::Ink, Center);
		if (rec.unlocked) {
			char b[32];
			std::snprintf(b, sizeof(b), "%d/%d", rec.bananas, rec.total);
			text(t, assets.regular, b, 15, V2(x + 36.f, y + 141.f), ui::Ink);
			text(t, assets.regular, rec.done ? formatTime(rec.time) : "--:--", 15, V2(x + cw * 0.5f, y + 176.f),
			     withAlpha(ui::Ink, 200), Center);
		} else {
			Canvas lock;
			lock.roundRect(x, y, cw, ch, 12.f, sf::Color(0x2E, 0x24, 0x18, 110));
			vineKnot(lock, V2(x + cw * 0.5f, y + 160.f), 1.1f, sf::Color(0x3F, 0x6B, 0x2A));
			lock.draw(t);
		}
	}
	text(t, assets.regular, "Arrows move \xC2\xB7 Space plays \xC2\xB7 Esc back", 16, V2(400.f, 566.f),
	     withAlpha(ui::Cream, 230), Center);
}

void Game::drawCards(sf::RenderTarget& t) {
	if (cards.empty()) return;
	const CardPage& p = cards[cardIdx];
	const Palette& pal = paletteFor(p.biome);
	// paper
	sf::RectangleShape paper(V2(VIEW_W, VIEW_H));
	paper.setTexture(&assets.paper);
	paper.setTextureRect(sf::IntRect(0, 0, 800, 600));
	t.draw(paper);
	Canvas c;
	c.rectV(0, 0, 800, 600, withAlpha(pal.skyBot, 40), withAlpha(pal.skyBot, 70));
	// double border
	sf::Color bd = pal.earthDark;
	auto frame = [&](float inset, float w) {
		c.rect(inset, inset, 800 - 2 * inset, w, bd);
		c.rect(inset, 600 - inset - w, 800 - 2 * inset, w, bd);
		c.rect(inset, inset, w, 600 - 2 * inset, bd);
		c.rect(800 - inset - w, inset, w, 600 - 2 * inset, bd);
	};
	frame(22.f, 3.f);
	frame(30.f, 1.5f);
	// corner leaves
	for (int k = 0; k < 4; ++k) {
		V2 cp(k % 2 ? 770.f : 30.f, k / 2 ? 570.f : 30.f);
		V2 d = norm(V2(k % 2 ? -1.f : 1.f, k / 2 ? -1.f : 1.f));
		c.leaf(cp, d, 26.f, 8.f, withAlpha(pal.grass, 200), withAlpha(pal.earthDark, 160));
	}
	float inA = std::min(1.f, cardT / 0.3f);
	float outA = cardOut >= 0.f ? std::max(0.f, 1.f - cardOut / 0.3f) : 1.f;
	float a = inA * outA;
	int ai = static_cast<int>(255 * a);
	// vignette art in the top third
	V2 art(400.f, 165.f);
	switch (p.art) {
	case 1: swirl(c, art, 1.4f, withAlpha(lerp(pal.mid, pal.earthDark, 0.3f), ai)); break;
	case 2:
		for (int k = 0; k < 8; ++k) {
			float an = clock * 0.3f + k * PI / 4.f;
			V2 d(std::cos(an), std::sin(an));
			c.tri(art, art + rot(d, 0.07f) * 120.f, art + rot(d, -0.07f) * 120.f, withAlpha(ui::GoldCore, static_cast<int>(70 * a)));
		}
		goldenLeaf(c, art, 90.f, 150.f, 0.3f, withAlpha(ui::GoldCore, ai), withAlpha(ui::GoldEdge, ai));
		break;
	case 3:
		for (int k = 0; k < 5; ++k) figIcon(c, art + V2((k - 2) * 44.f, (k % 2) * 12.f), 2.2f, true, a);
		break;
	case 4:
		goldenLeaf(c, art + V2(0.f, 20.f), 110.f, 280.f, 1.45f, withAlpha(sf::Color(0x58, 0xB3, 0x48), ai),
		           withAlpha(sf::Color(0x3F, 0x8E, 0x3A), ai));
		break;
	default:
		goldenLeaf(c, art, 80.f, 150.f, 0.5f, withAlpha(pal.grass, ai), withAlpha(lerp(pal.grass, pal.earthDark, 0.5f), ai));
		break;
	}
	c.draw(t);
	if (p.art == 4) {
		sf::Sprite pip(assets.monkey(PAnim::Stand, true));
		pip.setScale(2.f, 2.f);
		pip.setOrigin(32.f, 64.f);
		pip.setPosition(art.x + 6.f, art.y + 30.f + std::sin(clock * 1.2f) * 2.f);
		pip.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(ai)));
		t.draw(pip);
		// z z z
		for (int k = 0; k < 3; ++k) {
			float u = std::fmod(clock * 0.5f + k / 3.f, 1.f);
			text(t, assets.bold, "z", 14 + 4 * k, V2(art.x + 60.f + u * 30.f + k * 8.f, art.y - 20.f - u * 50.f - k * 10.f),
			     withAlpha(ui::Ink, static_cast<int>(200 * (1.f - u))), Left, a);
		}
	}
	// text
	float y0 = 300.f + (3 - static_cast<int>(p.lines.size())) * 16.f;
	unsigned size = (p.lines.size() && p.lines[0] == "THE END") ? 48 : 22;
	for (size_t i = 0; i < p.lines.size(); ++i) {
		unsigned sz = (i == 0) ? size : 22;
		text(t, i == 0 && size == 48 ? assets.bold : assets.regular, p.lines[i], sz, V2(400.f, y0 + i * (sz == 48 ? 70.f : 38.f)),
		     ui::Ink, Center, a);
	}
	float pulse = 0.55f + 0.45f * std::sin(clock * 3.f);
	text(t, assets.regular, "Space to continue", 16, V2(400.f, 530.f), withAlpha(pal.earthDark, 255), Center, pulse * inA);
}

void Game::drawHud(sf::RenderTarget& t) {
	if (!myWorld || hudAlpha < 0.01f) return;
	World& w = *myWorld;
	float a = hudAlpha;
	Canvas c;
	c.roundRect(12.f, 12.f, 262.f, 42.f, 12.f, withAlpha(ui::Cream, static_cast<int>(200 * a)));
	bananaIcon(c, V2(34.f, 34.f), 1.1f, a);
	figIcon(c, V2(146.f, 34.f), 1.05f, w.fig, a);
	skullLeafIcon(c, V2(190.f, 33.f), 1.f, a);
	c.roundRect(700.f, 12.f, 88.f, 32.f, 10.f, withAlpha(ui::Cream, static_cast<int>(170 * a)));
	c.draw(t);
	char buf[32];
	std::snprintf(buf, sizeof(buf), "%d / %d", w.bananas, w.bananasTotal);
	text(t, assets.bold, buf, 22, V2(50.f, 18.f), ui::Ink, Left, a);
	std::snprintf(buf, sizeof(buf), "%d", w.deaths);
	text(t, assets.bold, buf, 22, V2(206.f, 18.f), ui::Ink, Left, a);
	text(t, assets.regular, formatTime(w.levelTimer), 20, V2(744.f, 15.f), ui::Ink, Center, a);
}

void Game::drawSignBubble(sf::RenderTarget& t) {
	if (!myWorld || !myRenderer) return;
	Renderer& r = *myRenderer;
	if (r.nearSign < 0 || r.signAlpha <= 0.f || r.nearSign >= static_cast<int>(myWorld->signs.size())) return;
	const Sign& s = myWorld->signs[r.nearSign];
	float a = r.signAlpha;
	auto lines = wrap(assets.regular, s.text, 18, 296.f);
	float tw = 0.f;
	for (auto& l : lines) tw = std::max(tw, textWidth(assets.regular, l, 18));
	float w = std::min(320.f, tw + 28.f), h = lines.size() * 24.f + 18.f;
	V2 post = r.toScreen(b2Vec2(s.tx * TILE + 0.25f, s.ty * TILE - 0.05f));
	float x = std::clamp(post.x - w * 0.5f, 10.f, 790.f - w);
	float y = std::max(10.f, post.y - h - 20.f);
	Canvas c;
	c.roundRect(x + 3.f, y + 4.f, w, h, 12.f, sf::Color(0, 0, 0, static_cast<sf::Uint8>(50 * a)));
	c.roundRect(x, y, w, h, 12.f, withAlpha(ui::Cream, static_cast<int>(245 * a)));
	float tx = std::clamp(post.x, x + 16.f, x + w - 16.f);
	c.tri(V2(tx - 9.f, y + h - 1.f), V2(tx + 9.f, y + h - 1.f), V2(tx, y + h + 12.f), withAlpha(ui::Cream, static_cast<int>(245 * a)));
	c.draw(t);
	for (size_t i = 0; i < lines.size(); ++i)
		text(t, assets.regular, lines[i], 18, V2(x + w * 0.5f, y + 8.f + i * 24.f), ui::Ink, Center, a);
}

void Game::drawPause(sf::RenderTarget& t) {
	dim(t, 140);
	Canvas c;
	c.roundRect(236.f, 96.f, 336.f, 420.f, 18.f, sf::Color(0, 0, 0, 60));
	c.roundRect(230.f, 90.f, 340.f, 420.f, 18.f, ui::Cream);
	c.rect(262.f, 146.f, 276.f, 3.f, paletteFor(myLevel ? myLevel->biome : Biome::Canopy).accent);
	c.draw(t);
	text(t, assets.bold, "Paused", 34, V2(400.f, 102.f), ui::Ink, Center);
	std::vector<std::string> items = {"Resume", "Restart from checkpoint", "Restart level", "Level select", "Quit to title"};
	for (size_t i = 0; i < items.size(); ++i) {
		bool s = static_cast<int>(i) == pauseSel;
		V2 p(400.f, 168.f + i * 42.f);
		if (s) {
			Canvas h;
			h.roundRect(262.f, p.y - 4.f, 276.f, 36.f, 10.f, withAlpha(ui::Banana, 170));
			h.draw(t);
		}
		text(t, s ? assets.bold : assets.regular, items[i], 22, p, ui::Ink, Center);
	}
	const char* ctl[] = {"Arrows / A D: move   Space / Z / K: jump", "Up / Down: climb, swim, drop through branches",
	                     "R: restart from checkpoint   Esc: pause"};
	for (int i = 0; i < 3; ++i) text(t, assets.regular, ctl[i], 13, V2(400.f, 400.f + i * 20.f), withAlpha(ui::Ink, 200), Center);
	if (myWorld) {
		char buf[96];
		std::snprintf(buf, sizeof(buf), "Bananas %d/%d   Deaths %d   %s", myWorld->bananas, myWorld->bananasTotal, myWorld->deaths,
		              formatTime(myWorld->levelTimer).c_str());
		text(t, assets.bold, buf, 15, V2(400.f, 470.f), withAlpha(ui::Ink, 220), Center);
	}
}

void Game::drawResults(sf::RenderTarget& t) {
	dim(t, 120);
	const Palette& p = paletteFor(biomeOfLevel(levelIndex));
	Canvas c;
	c.roundRect(176.f, 86.f, 460.f, 440.f, 20.f, sf::Color(0, 0, 0, 70));
	c.roundRect(170.f, 80.f, 460.f, 440.f, 20.f, ui::Cream);
	c.rect(200.f, 142.f, 400.f, 3.f, p.accent);
	const LevelRecord& rec = save.levels[levelIndex];
	bananaIcon(c, V2(222.f, 190.f), 1.2f);
	figIcon(c, V2(222.f, 236.f), 1.1f, lastRun.fig);
	skullLeafIcon(c, V2(222.f, 280.f), 1.1f);
	// clock icon
	c.ring(V2(222.f, 324.f), 10.f, 3.f, ui::Ink, 18);
	c.line(V2(222.f, 324.f), V2(222.f, 317.f), 2.f, ui::Ink);
	c.line(V2(222.f, 324.f), V2(227.f, 326.f), 2.f, ui::Ink);
	c.draw(t);
	char buf[128];
	std::snprintf(buf, sizeof(buf), "LEVEL %d \xE2\x80\x94 %s", levelIndex, upper(levelName(levelIndex)).c_str());
	text(t, assets.bold, buf, 26, V2(400.f, 98.f), ui::Ink, Center);
	auto row = [&](float y, const std::string& label, const std::string& val, const std::string& best, bool nb) {
		text(t, assets.regular, label, 20, V2(248.f, y), ui::Ink);
		text(t, assets.bold, val, 20, V2(470.f, y), ui::Ink, Right);
		if (!best.empty()) text(t, assets.regular, best, 15, V2(484.f, y + 4.f), withAlpha(ui::Ink, 160));
		if (nb) {
			Canvas tag;
			tag.roundRect(538.f, y + 1.f, 76.f, 22.f, 8.f, ui::Danger);
			tag.draw(t);
			text(t, assets.bold, "New best!", 13, V2(576.f, y + 4.f), ui::Cream, Center);
		}
	};
	std::snprintf(buf, sizeof(buf), "%d / %d", lastRun.bananas, lastRun.total);
	char best[48];
	std::snprintf(best, sizeof(best), "best %d", rec.bananas);
	row(178.f, "Bananas", buf, best, lastBest.bananas);
	row(224.f, "Golden fig", lastRun.fig ? "found!" : "not found", rec.fig ? "found before" : "", lastBest.fig);
	std::snprintf(buf, sizeof(buf), "%d", lastRun.deaths);
	std::snprintf(best, sizeof(best), "best %d", rec.deaths);
	row(268.f, "Deaths", buf, best, lastBest.deaths);
	std::snprintf(best, sizeof(best), "best %s", formatTime(rec.time).c_str());
	row(312.f, "Time", formatTime(lastRun.time), best, lastBest.time);
	std::vector<std::string> items = {levelIndex < NUM_LEVELS ? "Next level" : "Credits", "Replay", "Level select"};
	for (size_t i = 0; i < items.size(); ++i) {
		bool s = static_cast<int>(i) == resultsSel;
		V2 pp(400.f, 380.f + i * 42.f);
		if (s) {
			Canvas h;
			h.roundRect(290.f, pp.y - 4.f, 220.f, 36.f, 10.f, withAlpha(ui::Banana, 170));
			h.draw(t);
		}
		text(t, s ? assets.bold : assets.regular, items[i], 22, pp, ui::Ink, Center);
	}
}

void Game::drawCredits(sf::RenderTarget& t) {
	Canvas bg;
	bg.rectV(0, 0, 800, 600, sf::Color(0x20, 0x55, 0x3C), sf::Color(0x0F, 0x2A, 0x20));
	bg.draw(t);
	creditLeaves.draw(t, assets.atlas);
	struct Line {
		const char* s;
		unsigned size;
		bool bold;
		float gap;
	};
	const Line lines[] = {
	    {"LEAFWIND", 56, true, 90.f},
	    {"A little monkey. A big wind. One leaf.", 20, false, 90.f},
	    {"a game by", 16, false, 26.f},
	    {"Alexander Schrab", 26, true, 70.f},
	    {"design", 16, false, 26.f},
	    {"the Leafwind design document", 22, true, 70.f},
	    {"code", 16, false, 26.f},
	    {"rebuilt in C++17 with SFML and Box2D", 22, true, 70.f},
	    {"Pip's sprites (2010)", 16, false, 26.f},
	    {"Alexander Schrab", 22, true, 70.f},
	    {"everything else", 16, false, 26.f},
	    {"procedurally drawn and synthesized at startup", 22, true, 70.f},
	    {"font", 16, false, 26.f},
	    {"DejaVu Sans", 22, true, 110.f},
	    {"thank you for playing", 30, true, 40.f},
	};
	float y = 620.f - screenT * 40.f;
	for (const Line& l : lines) {
		textShadow(t, l.bold ? assets.bold : assets.regular, l.s, l.size, V2(400.f, y), ui::Cream, sf::Color(0, 0, 0, 120),
		           V2(1.f, 2.f), Center);
		y += l.gap;
	}
	Canvas gl;
	goldenLeaf(gl, V2(400.f, y + 80.f), 60.f, 100.f, 0.3f, ui::GoldCore, ui::GoldEdge);
	gl.draw(t);
}
