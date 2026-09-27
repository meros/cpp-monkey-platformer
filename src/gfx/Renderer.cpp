#include "Renderer.h"

#include "core/Config.h"

#include <algorithm>
#include <cmath>

using namespace cfg;

namespace {
const float T = TILE_PX;
inline V2 px(b2Vec2 m) { return V2(m.x * PPM, m.y * PPM); }
} // namespace

V2 Renderer::toScreen(b2Vec2 m) const {
	V2 c = cam.center();
	return V2((m.x * PPM - c.x) * VIEW_ZOOM + VIEW_W * 0.5f, (m.y * PPM - c.y) * VIEW_ZOOM + VIEW_H * 0.5f);
}

void Renderer::setLevel(const LevelData& L, int seed) {
	level = &L;
	pal = paletteFor(L.biome);
	bg.build(L.biome, L.width * T, L.height * T, seed);
	terrain.build(L, assets, seed);
	buildWater(L);
	parts.clear();
	parts.seed(static_cast<unsigned>(seed) * 31u + 5u);
	cam.setLevelSize(L.width * T, L.height * T);
	nextLightning = 6.f + (seed % 5);
	lightningT = -1.f;
	lightning = 0.f;
	for (float& a : spawnAcc) a = 0.f;
}

void Renderer::buildWater(const LevelData& L) {
	water.clear();
	surfaces.clear();
	for (int x = 0; x < L.width; ++x) {
		int y = 0;
		while (y < L.height) {
			if (!L.waterAt(x, y)) {
				++y;
				continue;
			}
			int top = y;
			while (y < L.height && L.waterAt(x, y)) ++y;
			water.push_back({x, top, y - 1, L.waterAt(x, top)});
			surfaces.push_back({x, top});
		}
	}
}

// ---------------------------------------------------------------------------

void Renderer::handleEvents(World& w) {
	const sf::Color dustCol = withAlpha(lerp(pal.earth, pal.fog, 0.55f), 170);
	for (const GameEvent& e : w.events) {
		V2 p(e.x * PPM, e.y * PPM);
		switch (e.type) {
		case Ev::Jump:
			parts.dust(p, 3, 8.f, dustCol);
			break;
		case Ev::LandSoft:
			parts.dust(p, 4, 10.f, dustCol);
			break;
		case Ev::LandHard:
			parts.dust(p, parts.rndi(6, 10), 14.f, dustCol);
			cam.shake(2.f, 0.1f);
			break;
		case Ev::Step:
			if (parts.rnd(0.f, 1.f) < 0.3f) parts.dust(p, 1, 6.f, withAlpha(dustCol, 120));
			break;
		case Ev::Banana:
			parts.sparkles(p, 8, ui::Banana, 0.3f, 160.f);
			break;
		case Ev::Fig:
			parts.sparkles(p, 24, ui::GoldCore, 0.8f, 220.f);
			parts.ring(p, 40.f, withAlpha(ui::GoldCore, 200), 0.6f);
			break;
		case Ev::RopeGrab:
			parts.leafBurst(p, 3, pal.grass, pal.grassLight);
			break;
		case Ev::Splash:
			parts.splash(p, 12);
			break;
		case Ev::Drip:
			for (int i = 0; i < 4; ++i) {
				Particle& q = parts.spawn();
				q.kind = PKind::Drop;
				q.pos = p + V2(parts.rnd(-10.f, 10.f), parts.rnd(0.f, 16.f));
				q.vel = V2(0.f, parts.rnd(20.f, 60.f));
				q.gravity = 600.f;
				q.maxLife = 0.5f;
				q.size = 2.f;
				q.color = sf::Color(0xDC, 0xF5, 0xFF, 220);
			}
			break;
		case Ev::Death:
			parts.leafBurst(p, 20, pal.grass, pal.accent);
			break;
		case Ev::Checkpoint:
			parts.sparkles(p + V2(0.f, -40.f), 16, ui::GoldCore, 0.7f, 140.f);
			break;
		case Ev::ExitEnter:
			parts.sparkles(p, 12, ui::GoldCore, 0.6f, 120.f);
			break;
		case Ev::Bounce:
			parts.spores(p, e.value > 1.1f ? 18 : 10,
			             level->biome == Biome::Hollow ? sf::Color(0x8C, 0xF0, 0xD0) : sf::Color(0xFF, 0xE0, 0xB0));
			break;
		case Ev::Squish:
			parts.dust(p, 6, 10.f, dustCol);
			parts.sparkles(p, 6, sf::Color(0x9F, 0xB8, 0xFF), 0.4f, 120.f);
			break;
		case Ev::CrumbleFall:
			parts.dust(p + V2(0.f, 20.f), 8, 18.f, withAlpha(sf::Color(0x8E, 0x8C, 0x86), 200));
			break;
		case Ev::CrumbleReturn:
			parts.sparkles(p, 5, withAlpha(pal.grassLight, 200), 0.4f, 60.f);
			break;
		case Ev::LogThud:
			parts.dust(p, 12, 40.f, dustCol);
			cam.shake(3.f, 0.2f);
			break;
		case Ev::BoulderImpact:
			if (e.value > 4.f) {
				cam.shake(4.f, 0.25f);
				parts.dust(p, 8, 20.f, dustCol);
			}
			break;
		case Ev::GustStart:
			cam.shake(1.f, w.level.gustOn);
			break;
		default:
			break;
		}
	}
}

void Renderer::ambient(World& w, float dt, float time) {
	sf::FloatRect vr = cam.rect();
	const Biome b = w.level.biome;
	float windX = 0.f;
	if (w.level.gust) windX = (w.windEnvelope) * -260.f; // storm gusts: mostly leftward bands, see below
	// wind direction at the player (for ambient leaves)
	int wd = w.windDirAt(w.player.pos().x, w.player.pos().y);
	float ambientWind = wd * 200.f * w.windEnvelope;
	(void)windX;

	// ambient falling leaves: ~1 per 30 tile^2 visible
	spawnAcc[0] += dt * (300.f / 30.f) / 12.f;
	while (spawnAcc[0] >= 1.f) {
		spawnAcc[0] -= 1.f;
		Particle& q = parts.spawn();
		q.kind = PKind::Leaf;
		q.pos = V2(vr.left + parts.rnd(-100.f, vr.width + 100.f), vr.top - 20.f);
		q.vel = V2(parts.rnd(-10.f, 10.f), T * parts.rnd(0.8f, 1.2f));
		q.wobble = parts.rnd(20.f, 45.f);
		q.phase = parts.rnd(0.f, 6.f);
		q.rot = parts.rnd(0.f, 6.f);
		q.spin = parts.rnd(-1.5f, 1.5f);
		q.maxLife = 14.f;
		q.size = parts.rnd(5.f, 8.f);
		q.color = parts.rnd(0.f, 1.f) < 0.75f ? lerp(pal.grass, pal.grassLight, parts.rnd(0.f, 1.f)) : pal.accent;
		q.shrink = false;
		q.fadeIn = true;
		if (b == Biome::Hollow) q.color = withAlpha(lerp(pal.accent, pal.grassLight, 0.5f), 200);
	}
	// fireflies in dim biomes
	if (b == Biome::Hollow || b == Biome::Swamp || b == Biome::Storm) {
		spawnAcc[1] += dt * 20.f / 6.f;
		while (spawnAcc[1] >= 1.f) {
			spawnAcc[1] -= 1.f;
			Particle& q = parts.spawn();
			q.kind = PKind::Firefly;
			q.pos = V2(vr.left + parts.rnd(0.f, vr.width), vr.top + parts.rnd(0.f, vr.height));
			q.vel = V2(parts.rnd(-8.f, 8.f), parts.rnd(-8.f, 8.f));
			q.maxLife = 6.f;
			q.size = 2.5f;
			q.phase = parts.rnd(0.f, 6.f);
			q.color = sf::Color(0xDF, 0xF4, 0x8A, 230);
			q.additive = true;
			q.fadeIn = true;
			q.shrink = false;
		}
	}
	// wind streaks and updraft spirals in visible wind tiles
	const LevelData& L = w.level;
	int x0 = std::max(0, static_cast<int>(vr.left / T)), x1 = std::min(L.width - 1, static_cast<int>((vr.left + vr.width) / T));
	int y0 = std::max(0, static_cast<int>(vr.top / T)), y1 = std::min(L.height - 1, static_cast<int>((vr.top + vr.height) / T));
	bool warn = L.gust && !w.gustOn && w.windEnvelope > 0.f;
	bool active = w.windActive() || warn;
	if (active) {
		float strength = L.gust ? std::max(0.35f, w.windEnvelope) : 1.f;
		for (int y = y0; y <= y1; ++y) {
			for (int x = x0; x <= x1; ++x) {
				uint8_t wc = L.windAt(x, y);
				if (!wc) continue;
				float rate = (wc == WI_UP ? 0.35f : 0.4f) * strength;
				if (parts.rnd(0.f, 1.f) > rate * dt) continue;
				Particle& q = parts.spawn();
				q.pos = V2(x * T + parts.rnd(0.f, T), y * T + parts.rnd(0.f, T));
				if (wc == WI_UP) {
					q.kind = PKind::Leaf;
					q.vel = V2(0.f, -T * parts.rnd(3.f, 5.f));
					q.wobble = 60.f;
					q.phase = parts.rnd(0.f, 6.f);
					q.spin = 5.f;
					q.maxLife = 1.6f;
					q.size = parts.rnd(4.f, 6.f);
					q.color = lerp(pal.grass, pal.grassLight, parts.rnd(0.f, 1.f));
					q.fadeIn = true;
					q.shrink = false;
				} else {
					int dir = wc == WI_LEFT ? -1 : 1;
					bool leaf = parts.rnd(0.f, 1.f) < 0.3f;
					q.kind = leaf ? PKind::Leaf : PKind::Streak;
					q.vel = V2(dir * T * parts.rnd(6.f, 10.f), parts.rnd(-10.f, 10.f));
					q.maxLife = parts.rnd(0.5f, 0.9f);
					q.size = leaf ? parts.rnd(4.f, 6.f) : parts.rnd(14.f, 26.f);
					q.spin = leaf ? 8.f * dir : 0.f;
					q.color = leaf ? lerp(pal.grass, pal.accent, parts.rnd(0.f, 0.4f)) : withAlpha(sf::Color::White, 120);
					q.fadeIn = true;
					q.shrink = false;
				}
			}
		}
	}
	// running dust while moving on ground (2 puffs/s)
	if (w.player.state == PState::Ground && std::fabs(w.player.vel().x) > 2.f) {
		spawnAcc[2] += dt * 2.f;
		if (spawnAcc[2] >= 1.f) {
			spawnAcc[2] -= 1.f;
			parts.dust(V2(w.player.pos().x * PPM - w.player.facing * 10.f, w.player.bottom() * PPM), 1, 4.f,
			           withAlpha(lerp(pal.earth, pal.fog, 0.55f), 140));
		}
	}
	// crumbling ledges shed pebbles while shaking
	for (const auto& c : w.crumbles) {
		if (c->state == Crumble::Shaking && parts.rnd(0.f, 1.f) < 12.f * dt)
			parts.pebbles(V2(c->tx * T + T * 0.5f, (c->ty + 1) * T), 1, sf::Color(0x6E, 0x6C, 0x66));
	}
	// golden leaf sparkles
	if (b == Biome::Storm) {
		spawnAcc[3] += dt * 3.f;
		while (spawnAcc[3] >= 1.f) {
			spawnAcc[3] -= 1.f;
			V2 lp(w.exitX * T + T * 0.5f, w.exitY * T - T * 0.5f);
			parts.sparkles(lp + V2(parts.rnd(-40.f, 40.f), parts.rnd(-50.f, 50.f)), 1, ui::GoldCore, 1.f, 30.f);
		}
		// lightning
		if (time >= nextLightning) {
			lightningT = 0.f;
			nextLightning = time + parts.rnd(12.f, 20.f);
			thunder = true; // picked up by the audio layer (low rumble)
		}
		if (lightningT >= 0.f) {
			lightningT += dt;
			// double flicker inside a 1.5 s window, each flash 0.1 s
			float f = 0.f;
			if (lightningT < 0.1f) f = 1.f;
			else if (lightningT > 0.25f && lightningT < 0.35f) f = 0.6f;
			lightning = f;
			if (lightningT > 1.5f) lightningT = -1.f;
		} else {
			lightning = 0.f;
		}
	}
	parts.update(dt, ambientWind);
}

void Renderer::update(World& w, float dt, float time) {
	handleEvents(w);
	cam.update(w, dt);
	ambient(w, dt, time);
	// signpost bubble
	int best = -1;
	float bestD = 1e9f;
	b2Vec2 p = w.player.pos();
	for (size_t i = 0; i < w.signs.size(); ++i) {
		float sx = w.signs[i].tx * TILE + 0.25f, sy = w.signs[i].ty * TILE + 0.25f;
		float d = std::hypot(sx - p.x, sy - p.y);
		if (d < 1.5f * TILE + 0.3f && d < bestD) {
			bestD = d;
			best = static_cast<int>(i);
		}
	}
	if (best >= 0 && w.player.alive()) {
		if (best != nearSign && signAlpha > 0.f) signAlpha = std::max(0.f, signAlpha - dt / 0.15f);
		else {
			nearSign = best;
			signAlpha = std::min(1.f, signAlpha + dt / 0.15f);
		}
	} else {
		signAlpha = std::max(0.f, signAlpha - dt / 0.15f);
	}
}

// ---------------------------------------------------------------------------
// Water

void Renderer::drawWaterBack(sf::RenderTarget& t, float) const {
	Canvas c;
	sf::Color top = withAlpha(pal.water, 200), bot = withAlpha(scale(pal.water, 0.7f), 215);
	for (const WaterCol& col : water) {
		float x0 = col.x * T;
		float y0 = col.top * T + 4.f, y1 = (col.bottom + 1) * T;
		float depthRows = std::max(1.f, static_cast<float>(col.bottom - col.top + 1));
		sf::Color b2 = lerp(top, bot, std::min(1.f, depthRows / 5.f));
		c.rectV(x0, y0, T, y1 - y0, top, b2);
	}
	c.draw(t);
}

void Renderer::drawWaterFront(sf::RenderTarget& t, float time) const {
	Canvas c;
	sf::Color front = withAlpha(lerp(pal.water, pal.fog, 0.2f), 110);
	sf::Color front0 = withAlpha(pal.water, 0);
	sf::Color light(0xDC, 0xF5, 0xFF, 180);
	sf::Color darkLine = withAlpha(scale(pal.water, 0.75f), 180);
	auto surfY = [&](float xpx, float base) {
		float x = xpx / T;
		return base + 3.f * std::sin(x * 1.3f + time * 2.f) + 2.f * std::sin(x * 3.1f - time * 3.4f);
	};
	for (const WaterCol& col : water) {
		float x0 = col.x * T;
		float base = col.top * T + 4.f;
		const int N = 5;
		for (int i = 0; i < N; ++i) {
			float xa = x0 + T * i / N, xb = x0 + T * (i + 1) / N;
			float ya = surfY(xa, base), yb = surfY(xb, base);
			c.quad(V2(xa, ya), V2(xb, yb), V2(xb, base + 16.f), V2(xa, base + 16.f), front, front, front0, front0);
			// fill the gap between the static back-pass top and the wave
			c.line(V2(xa, ya + 1.5f), V2(xb, yb + 1.5f), 3.f, light);
			c.line(V2(xa, ya + 4.f), V2(xb, yb + 4.f), 2.f, darkLine);
		}
		// current streaks drifting with the flow
		for (int y = col.top; y <= col.bottom; ++y) {
			uint8_t f = level->waterAt(col.x, y);
			if (f != W_LEFT && f != W_RIGHT) continue;
			float h = hash1(col.x, y, 71);
			if (h > 0.3f * 2.f) continue;
			int dir = f == W_RIGHT ? 1 : -1;
			float ph = std::fmod(h * 97.f * T + time * 3.f * T * dir, T);
			if (ph < 0.f) ph += T;
			float sx = x0 + ph;
			float sy = y * T + 8.f + hash1(col.x, y, 5) * 24.f;
			float fade = std::sin(ph / T * 3.14159f);
			c.line(V2(sx - 8.f * dir, sy), V2(sx, sy), 1.5f, withAlpha(sf::Color(0xDC, 0xF5, 0xFF), static_cast<int>(150 * fade)));
		}
	}
	c.draw(t);
}

// ---------------------------------------------------------------------------

void Renderer::drawPlayer(sf::RenderTarget& t, const World& w, float time) const {
	if (hidePlayer) return;
	const Player& p = w.player;
	const sf::Texture& tex = assets.monkey(p.anim, p.facing > 0);
	sf::Sprite s(tex);
	b2Vec2 c = p.pos();
	V2 centre(c.x * PPM, c.y * PPM);
	// frame bottom-centre on the collider bottom-centre; origin at the collider centre
	s.setOrigin(32.f, 64.f - PLAYER_H * 0.5f * PPM);
	s.setPosition(std::round(centre.x), std::round(centre.y));
	float sx = 1.f, sy = 1.f;
	float angle = 0.f;
	sf::Uint8 alpha = 255;
	if (p.state == PState::Rope) {
		angle = p.spriteAngle * 180.f / 3.14159f;
		s.setOrigin(32.f, 32.f);
	}
	if (p.squashTimer > 0.f) {
		sx = 1.15f;
		sy = 0.85f;
	}
	if (p.state == PState::Swim) s.move(0.f, std::round(2.f * std::sin(time * 4.f)));
	if (p.state == PState::Dead) {
		float u = std::min(1.f, p.deathSpin / DEATH_ANIM);
		angle = u * 540.f * (p.facing > 0 ? 1.f : -1.f);
		sx = sy = 1.f - u * 0.9f;
		s.setOrigin(32.f, 40.f);
		if (p.deathSpin > DEATH_ANIM) alpha = 0;
	}
	if (p.state == PState::Exiting) alpha = static_cast<sf::Uint8>(255 * std::clamp(p.fade, 0.f, 1.f));
	s.setRotation(angle);
	s.setScale(sx, sy);
	s.setColor(sf::Color(255, 255, 255, alpha));
	// soft contact shadow
	if (p.state == PState::Ground) {
		Canvas sh;
		sh.ellipse(V2(centre.x, p.bottom() * PPM - 1.f), 20.f, 4.f, sf::Color(0, 0, 0, 45));
		sh.draw(t);
	}
	t.draw(s);
}

void Renderer::drawRain(sf::RenderTarget& t, const World& w, float time) const {
	if (w.level.biome != Biome::Storm) return;
	SpriteBatch b;
	float gust = w.gustOn ? 1.f : 0.3f;
	float ang = (-20.f * gust) * 3.14159f / 180.f; // leans with the (leftward) gusts
	V2 dir(std::sin(ang), std::cos(ang));
	float speed = 25.f * T;
	for (int i = 0; i < 200; ++i) {
		float h1 = hash1(i, 1, 3), h2 = hash1(i, 2, 3);
		float y = std::fmod(h2 * 700.f + time * speed, 700.f) - 50.f;
		float x = std::fmod(h1 * 1000.f + y * (-dir.x / dir.y) + 1000.f, 1000.f) - 100.f;
		V2 a(x, y);
		b.addStreak(a - dir * 18.f, a, 1.2f, sf::Color(200, 210, 230, 90));
	}
	b.draw(t, assets.atlas, sf::BlendAlpha);
}

void Renderer::drawWorld(sf::RenderTarget& t, const World& w, float time) {
	sf::View screen(sf::FloatRect(0.f, 0.f, VIEW_W, VIEW_H));
	t.setView(screen);
	V2 cc = cam.center();
	bg.drawSky(t, cc, time, lightning);
	bg.drawLayers(t, cc, time);
	bg.drawShafts(t, cc, time);

	sf::View wv = cam.view();
	t.setView(wv);
	sf::FloatRect vr = cam.rect();
	drawWaterBack(t, time);
	terrain.drawStatic(t, vr);

	float wind = w.windDirAt(w.player.pos().x, w.player.pos().y) * w.windEnvelope;
	if (w.level.biome == Biome::Cliffs || w.level.biome == Biome::Storm) wind = (w.level.gust ? -w.windEnvelope : -0.4f);
	Canvas c;
	SpriteBatch glow;
	drawDecor(c, glow, w, time);
	c.draw(t);
	c.clear();
	terrain.drawDynamic(t, vr, time, wind);
	drawObjects(c, glow, w, time);
	drawItems(c, glow, w, time);
	c.draw(t);
	drawPlayer(t, w, time);
	drawWaterFront(t, time);
	parts.draw(t, assets.atlas);

	// lights (additive)
	const Biome b = w.level.biome;
	if (b == Biome::Hollow || b == Biome::Mill || b == Biome::Storm) {
		V2 pp(w.player.pos().x * PPM, w.player.pos().y * PPM);
		glow.add(Tex::Glow, pp, V2(4.f * T, 4.f * T), 0.f, sf::Color(0xFF, 0xE9, 0xB0, 35));
	}
	glow.draw(t, assets.atlas, sf::BlendAdd);

	// screen-space overlays
	t.setView(screen);
	bg.drawForeground(t, cc, toScreen(w.player.pos()), time);
	drawRain(t, w, time);
	sf::Sprite vig(assets.vignette);
	vig.setScale(VIEW_W / assets.vignette.getSize().x, VIEW_H / assets.vignette.getSize().y);
	int va = (b == Biome::Hollow || b == Biome::Storm) ? 140 : 90;
	vig.setColor(sf::Color(0x1A, 0x12, 0x08, static_cast<sf::Uint8>(va)));
	t.draw(vig);
	if (lightning > 0.f) {
		Canvas f;
		f.rect(0, 0, VIEW_W, VIEW_H, sf::Color(255, 255, 255, static_cast<sf::Uint8>(40 * lightning)));
		f.draw(t);
	}
}
