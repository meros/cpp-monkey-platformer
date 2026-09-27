#include "Background.h"

#include "Assets.h"

#include <algorithm>
#include <cmath>
#include <random>

namespace {

const float PI = 3.14159265f;
const float F_FAR = 0.15f, F_MID = 0.4f, F_NEAR = 0.7f, F_FG = 1.3f;

struct Rng {
	std::mt19937 g;
	explicit Rng(unsigned s) : g(s) {}
	float operator()(float a, float b) { return std::uniform_real_distribution<float>(a, b)(g); }
	int i(int a, int b) { return std::uniform_int_distribution<int>(a, b)(g); }
};

// Smoothed 1D noise from a few octaves of sines with random phases.
struct Ridge {
	float ph[4], fr[4], am[4];
	Ridge(Rng& r, float scale) {
		for (int k = 0; k < 4; ++k) {
			ph[k] = r(0.f, 6.28f);
			fr[k] = scale * (1 << k) * r(0.8f, 1.2f);
			am[k] = 1.f / (1 << k);
		}
	}
	float operator()(float x) const {
		float v = 0.f, t = 0.f;
		for (int k = 0; k < 4; ++k) {
			v += std::sin(x * fr[k] + ph[k]) * am[k];
			t += am[k];
		}
		return v / t;
	}
};

void fillProfile(Canvas& c, const std::vector<V2>& pts, sf::Color top, sf::Color bottom, float bottomY) {
	for (size_t i = 0; i + 1 < pts.size(); ++i) {
		V2 a = pts[i], b = pts[i + 1];
		c.quad(a, b, V2(b.x, bottomY), V2(a.x, bottomY), top, top, bottom, bottom);
	}
}

void tree(Canvas& c, Rng& r, float x, float base, float h, sf::Color col, sf::Color hi, bool palm) {
	float w = h * r(0.05f, 0.08f);
	V2 top(x + r(-10.f, 10.f), base - h);
	c.taper(V2(x, base + 40.f), top, w * 1.5f, w * 0.6f, col);
	if (palm) {
		for (int i = 0; i < 7; ++i) {
			float a = -PI + (i + 0.5f) * PI / 7.f + r(-0.1f, 0.1f);
			std::vector<V2> pts;
			float L = h * r(0.35f, 0.45f);
			for (int k = 0; k <= 6; ++k) {
				float t = k / 6.f;
				V2 d(std::cos(a) * L * t, std::sin(a) * L * t * 0.6f + t * t * L * 0.45f);
				pts.push_back(top + d);
			}
			for (size_t k = 0; k + 1 < pts.size(); ++k)
				c.taper(pts[k], pts[k + 1], 14.f * (1.f - k / 6.f) + 2.f, 14.f * (1.f - (k + 1) / 6.f) + 2.f, col);
		}
		return;
	}
	int n = r.i(3, 5);
	for (int i = 0; i < n; ++i) {
		float rr = r(60.f, 110.f) * (h / 320.f);
		V2 p = top + V2(r(-1.f, 1.f) * rr * 0.9f, r(-0.6f, 0.4f) * rr);
		c.circle(p, rr, col);
		c.circle(p + V2(-rr * 0.18f, -rr * 0.2f), rr * 0.7f, hi);
		c.circle(p + V2(rr * 0.05f, rr * 0.05f), rr * 0.62f, col);
	}
}

void mushroomTree(Canvas& c, Rng& r, float x, float base, float h, sf::Color col, sf::Color hi) {
	float w = h * 0.08f;
	c.taper(V2(x, base + 40.f), V2(x + r(-15.f, 15.f), base - h), w * 1.4f, w, col);
	float cw = h * r(0.35f, 0.5f);
	V2 capC(x, base - h);
	std::vector<V2> cap;
	for (int i = 0; i <= 16; ++i) {
		float a = PI + PI * i / 16.f;
		cap.push_back(capC + V2(std::cos(a) * cw, std::sin(a) * cw * 0.55f));
	}
	c.polygon(cap, col);
	c.ellipse(capC + V2(-cw * 0.2f, -cw * 0.3f), cw * 0.35f, cw * 0.12f, hi);
}

} // namespace

float Background::yOffset(V2 cam, float f) const {
	float camBottom = std::max(300.f, levelH - 300.f);
	return (camBottom - cam.y) * f * 0.6f;
}

void Background::build(Biome biome, float W, float H, int seed) {
	myBiome = biome;
	pal = paletteFor(biome);
	levelW = W;
	levelH = H;
	Rng r(static_cast<unsigned>(seed) * 7919u + 13u);
	farBack.clear();
	farFront.clear();
	mid.clear();
	nearL.clear();
	fg.clear();
	clouds.clear();

	const bool hills = biome == Biome::River || biome == Biome::Swamp || biome == Biome::Canopy || biome == Biome::Grove;
	const bool peaks = biome == Biome::Cliffs || biome == Biome::Mill || biome == Biome::Gully ||
	                   biome == Biome::Ridge || biome == Biome::Storm;
	const bool hollow = biome == Biome::Hollow;

	// ---- far (two ridgelines) ----
	for (int layer = 0; layer < 2; ++layer) {
		Canvas& c = layer == 0 ? farBack : farFront;
		float x0 = -400.f, x1 = W * F_FAR + 1200.f;
		sf::Color col = lerp(pal.far, pal.fog, layer == 0 ? 0.55f : 0.25f);
		col.a = 200;
		sf::Color colB = lerp(col, pal.fog, 0.5f);
		colB.a = 200;
		float baseY = layer == 0 ? 290.f : 350.f;
		float amp = peaks ? 110.f : 60.f;
		Ridge rd(r, 1.f / 180.f);
		std::vector<V2> pts;
		int n = static_cast<int>((x1 - x0) / (800.f / 16.f)) + 1; // ~16 points per screen
		for (int i = 0; i <= n; ++i) {
			float x = x0 + (x1 - x0) * i / n;
			float y = baseY - amp * rd(x);
			if (peaks) y += (i % 2 ? 1.f : -1.f) * r(10.f, 40.f); // jagged
			pts.push_back(V2(x, y));
		}
		if (!peaks) pts = catmullRom(pts, 6);
		fillProfile(c, pts, col, colB, 2600.f);
		if (hills && layer == 1) {
			// tree-line bumps on the rounded hills
			for (float x = x0; x < x1; x += r(30.f, 60.f)) {
				float y = baseY - amp * rd(x) + 6.f;
				c.circle(V2(x, y), r(14.f, 28.f), col, 14);
			}
		}
		if (hollow) {
			// giant mushroom silhouettes
			for (float x = x0; x < x1; x += r(160.f, 320.f)) {
				float h = r(120.f, 260.f) * (layer == 0 ? 0.8f : 1.f);
				mushroomTree(c, r, x, baseY + 40.f, h, col, lerp(col, pal.accent, 0.15f));
			}
		}
		if (peaks && layer == 0 && biome != Biome::Storm) {
			// snow/light caps on the far peaks
			for (size_t i = 1; i + 1 < pts.size(); ++i) {
				if (pts[i].y < pts[i - 1].y && pts[i].y < pts[i + 1].y && pts[i].y < baseY - amp * 0.4f) {
					sf::Color snow = withAlpha(lerp(pal.fog, sf::Color::White, 0.5f), 150);
					V2 a = pts[i], l = pts[i - 1], rr = pts[i + 1];
					c.tri(a, a + (l - a) * 0.3f, a + (rr - a) * 0.3f, snow);
				}
			}
		}
	}

	// ---- mid: tree silhouettes ----
	{
		float x0 = -400.f, x1 = W * F_MID + 1200.f;
		sf::Color col = lerp(pal.mid, pal.fog, 0.12f);
		sf::Color hi = lerp(col, pal.grassLight, 0.12f);
		sf::Color groundTop = col, groundBot = lerp(col, pal.fog, 0.35f);
		const float base = 470.f;
		int trees = std::max(8, static_cast<int>(W / 40.f / 4.f)); // 1 tree per 4 tiles of level width
		float spacing = (x1 - x0) / trees;
		int idx = 0;
		for (float x = x0; x < x1; x += spacing * r(0.6f, 1.4f)) {
			float h = r(220.f, 360.f);
			if (hollow) mushroomTree(mid, r, x, base, h * 0.8f, col, lerp(col, pal.accent, 0.2f));
			else if (biome == Biome::Cliffs || biome == Biome::Gully || biome == Biome::Ridge) {
				if (r(0.f, 1.f) < 0.5f) tree(mid, r, x, base, h * 0.7f, col, hi, false);
				else {
					// rock spire
					float w = r(40.f, 90.f);
					mid.quad(V2(x - w, base + 40.f), V2(x - w * 0.3f, base - h * 0.8f), V2(x + w * 0.4f, base - h * 0.75f),
					         V2(x + w, base + 40.f), col);
				}
			} else tree(mid, r, x, base, h, col, hi, (++idx % 5) == 0);
		}
		if (biome == Biome::Mill) {
			for (float x = x0 + 300.f; x < x1; x += r(700.f, 1100.f)) {
				// old water-wheel tower
				mid.rect(x - 30.f, base - 260.f, 60.f, 300.f, col);
				mid.tri(V2(x - 45.f, base - 260.f), V2(x + 45.f, base - 260.f), V2(x, base - 320.f), col);
				V2 wc(x + 55.f, base - 60.f);
				mid.ring(wc, 70.f, 10.f, col);
				for (int k = 0; k < 8; ++k) mid.line(wc, wc + rot(V2(70.f, 0.f), k * PI / 4.f), 6.f, col);
			}
		}
		mid.rectV(x0, base, x1 - x0, 60.f, groundTop, groundTop);
		mid.rectV(x0, base + 60.f, x1 - x0, 2400.f, groundTop, groundBot);
		// soft bumps along the mid ground line
		for (float x = x0; x < x1; x += r(40.f, 90.f)) mid.circle(V2(x, base + 8.f), r(18.f, 40.f), col, 14);
	}

	// ---- near: trunks, hanging vines, leaf clusters ----
	{
		float x0 = -400.f, x1 = W * F_NEAR + 1200.f;
		sf::Color col = pal.near;
		sf::Color colLeaf = lerp(pal.near, pal.mid, 0.25f);
		const float base = 560.f;
		for (float x = x0; x < x1; x += r(260.f, 620.f)) {
			if (biome == Biome::Cliffs || biome == Biome::Storm || biome == Biome::Gully) {
				// jagged rock column
				float w = r(50.f, 110.f);
				std::vector<V2> pts = {V2(x - w, base + 2000.f)};
				float y = base + 60.f;
				float top = r(-120.f, 160.f);
				for (int k = 0; k < 6; ++k) {
					float t = k / 5.f;
					y = base + 60.f + (top - base - 60.f) * t;
					pts.push_back(V2(x - w * (1.f - 0.4f * t) + r(-10.f, 10.f), y));
				}
				pts.push_back(V2(x + w * 0.5f, top - 20.f));
				for (int k = 5; k >= 0; --k) {
					float t = k / 5.f;
					y = base + 60.f + (top - base - 60.f) * t;
					pts.push_back(V2(x + w * (1.f - 0.3f * t) + r(-10.f, 10.f), y));
				}
				pts.push_back(V2(x + w, base + 2000.f));
				nearL.polygon(pts, withAlpha(col, 235));
				if (biome == Biome::Storm) tree(nearL, r, x, top + 20.f, 140.f, withAlpha(col, 235), withAlpha(colLeaf, 235), false);
				continue;
			}
			if (biome == Biome::Mill) {
				// wooden posts with sagging ropes
				nearL.rect(x - 14.f, -3000.f, 28.f, 5600.f, withAlpha(col, 235));
				nearL.rect(x - 22.f, 60.f, 44.f, 14.f, withAlpha(col, 235));
				auto sag = quadBezier(V2(x, 80.f), V2(x + 180.f, 200.f), V2(x + 380.f, 80.f), 12);
				nearL.polyline(sag, 4.f, withAlpha(col, 220), false);
				continue;
			}
			float w = r(34.f, 70.f);
			// trunk rising through the whole screen, slight lean
			float lean = r(-40.f, 40.f);
			nearL.quad(V2(x - w * 0.6f, base + 2000.f), V2(x + w * 0.6f, base + 2000.f), V2(x + w * 0.45f + lean, -3000.f),
			           V2(x - w * 0.45f + lean, -3000.f), withAlpha(col, 235));
			// roots flare
			nearL.tri(V2(x - w * 1.5f, base + 30.f), V2(x - w * 0.4f, base - 40.f), V2(x, base + 30.f), withAlpha(col, 235));
			nearL.tri(V2(x + w * 1.5f, base + 30.f), V2(x + w * 0.4f, base - 40.f), V2(x, base + 30.f), withAlpha(col, 235));
			// hanging vines
			int nv = r.i(1, 3);
			for (int k = 0; k < nv; ++k) {
				float vx = x + r(-160.f, 160.f), L = r(120.f, 300.f);
				auto pts = quadBezier(V2(vx, -60.f), V2(vx + r(-40.f, 40.f), L * 0.6f), V2(vx + r(-15.f, 15.f), L), 10);
				nearL.polyline(pts, 3.f, withAlpha(col, 220), false);
				for (size_t q = 2; q < pts.size(); q += 3)
					nearL.leaf(pts[q], norm(V2((q % 2) ? 1.f : -1.f, 0.6f)), 14.f, 5.f, withAlpha(colLeaf, 220));
			}
			// leaf cluster near the top of the view
			if (!hollow) {
				V2 cc(x + r(-60.f, 60.f), r(-40.f, 60.f));
				for (int k = 0; k < 12; ++k) {
					float a = r(0.f, 2.f * PI);
					nearL.leaf(cc + V2(std::cos(a), std::sin(a)) * r(0.f, 40.f), V2(std::cos(a), std::sin(a)), r(40.f, 70.f),
					           r(12.f, 20.f), withAlpha(colLeaf, 230));
				}
			}
		}
	}

	// ---- foreground items ----
	for (float x = -200.f; x < W * F_FG + 800.f; x += r(380.f, 900.f))
		fg.push_back({x, r(0.7f, 1.2f), r.i(0, 2), r(0.f, 6.f)});

	// ---- clouds ----
	if (!hollow) {
		for (float x = -300.f; x < W * 0.08f + 1200.f; x += r(220.f, 420.f))
			clouds.push_back({V2(x, r(40.f, 200.f)), r(80.f, 170.f), r.i(0, 1000)});
	}
	shafts = biome == Biome::Canopy || biome == Biome::Grove || biome == Biome::Hollow;
}

void Background::drawSky(sf::RenderTarget& t, V2 cam, float time, float flash) const {
	Canvas c;
	sf::Color top = pal.skyTop, bot = pal.skyBot;
	if (flash > 0.f) {
		top = lerp(top, sf::Color::White, flash * 0.4f);
		bot = lerp(bot, sf::Color::White, flash * 0.4f);
	}
	// gradient shifts with height in tall levels: higher = more skyTop
	float h = std::clamp(-yOffset(cam, 1.f) / 1500.f, -0.3f, 0.f);
	c.rectV(0.f, 0.f, 800.f, 600.f, top, lerp(bot, top, -h));
	// sun glow
	if (myBiome != Biome::Storm && myBiome != Biome::Hollow)
		c.circleGrad(V2(620.f - cam.x * 0.02f, 120.f + yOffset(cam, 0.05f)), 260.f, withAlpha(lerp(bot, sf::Color::White, 0.6f), 110),
		             withAlpha(bot, 0), 40);
	// clouds (parallax 0.08, slow drift)
	for (const Cloud& cl : clouds) {
		float x = cl.pos.x - cam.x * 0.08f + std::fmod(time * 6.f, 4000.f) * 0.2f;
		x = std::fmod(x + 600.f, W_WRAP()) - 300.f;
		V2 p(x, cl.pos.y + yOffset(cam, 0.08f));
		sf::Color cc = withAlpha(lerp(pal.skyBot, sf::Color::White, 0.55f), 120);
		sf::Color sh = withAlpha(lerp(pal.skyBot, pal.skyTop, 0.5f), 90);
		for (int k = 0; k < 5; ++k) {
			float kx = (k - 2) * cl.w * 0.28f;
			float rr = cl.w * (0.28f + 0.1f * std::sin(cl.seed + k * 1.7f));
			c.circle(p + V2(kx, 8.f), rr * 0.9f, sh, 20);
		}
		for (int k = 0; k < 5; ++k) {
			float kx = (k - 2) * cl.w * 0.28f;
			float rr = cl.w * (0.28f + 0.1f * std::sin(cl.seed + k * 1.7f));
			c.circle(p + V2(kx, -rr * 0.3f), rr, cc, 20);
		}
	}
	if (myBiome == Biome::Storm) {
		// slow-scrolling dark cloud band
		sf::Color band = sf::Color(20, 24, 36, 60);
		for (int k = 0; k < 14; ++k) {
			float x = std::fmod(k * 90.f - time * 12.f - cam.x * 0.05f, 1260.f);
			if (x < -100.f) x += 1260.f;
			c.circle(V2(x - 100.f, 70.f + 25.f * std::sin(k * 1.3f)), 90.f, band, 24);
		}
	}
	c.draw(t);
}

float Background::W_WRAP() const { return std::max(1400.f, levelW * 0.08f + 1400.f); }

void Background::drawLayers(sf::RenderTarget& t, V2 cam, float) const {
	sf::RenderStates st;
	st.transform.translate(-cam.x * F_FAR + 400.f, yOffset(cam, F_FAR));
	farBack.draw(t, st);
	farFront.draw(t, st);
	st = sf::RenderStates();
	st.transform.translate(-cam.x * F_MID + 400.f, yOffset(cam, F_MID));
	mid.draw(t, st);
	st = sf::RenderStates();
	st.transform.translate(-cam.x * F_NEAR + 400.f, yOffset(cam, F_NEAR));
	nearL.draw(t, st);
	// haze over the background so the playfield pops
	Canvas haze;
	haze.rectV(0.f, 0.f, 800.f, 600.f, withAlpha(pal.fog, 0), withAlpha(pal.fog, 40));
	haze.draw(t);
}

void Background::drawShafts(sf::RenderTarget& t, V2 cam, float time) const {
	if (!shafts) return;
	Canvas c;
	sf::Color col(0xFF, 0xF6, 0xC8, 34);
	sf::Color clear(0xFF, 0xF6, 0xC8, 0);
	for (int k = 0; k < 4; ++k) {
		float sway = std::sin(time * 2.f * PI / 6.f + k) * (2.f * PI / 180.f);
		float x = std::fmod(180.f + k * 230.f - cam.x * 0.2f, 1000.f);
		if (x < -100.f) x += 1000.f;
		float w = 50.f + 25.f * (k % 2);
		float ang = 0.42f + sway;
		V2 dir(-std::sin(ang), std::cos(ang));
		V2 a(x, -20.f), b(x + w, -20.f);
		float L = 760.f;
		sf::Color cc = col;
		cc.a = static_cast<sf::Uint8>(25 + 15 * (k % 2));
		c.quad(a, b, b + dir * L, a + dir * L, cc, cc, clear, clear);
	}
	sf::RenderStates st;
	st.blendMode = sf::BlendAdd;
	c.draw(t, st);
}

void Background::drawForeground(sf::RenderTarget& t, V2 cam, V2 playerScreen, float time) const {
	Canvas c;
	sf::Color col = withAlpha(lerp(pal.near, sf::Color::Black, 0.35f), 170);
	sf::Color col2 = withAlpha(lerp(pal.near, pal.grass, 0.25f), 170);
	float yo = yOffset(cam, F_FG) * 0.3f;
	for (const FgItem& f : fg) {
		float sx = f.x - cam.x * F_FG + 400.f;
		if (sx < -150.f || sx > 950.f) continue;
		if (std::fabs(sx - playerScreen.x) < 160.f + 60.f * f.scale) continue;
		float baseY = 600.f + 12.f + std::max(0.f, yo);
		float sway = std::sin(time * 1.3f + f.phase) * 0.06f;
		float s = f.scale;
		if (f.kind == 0) {
			// fern: fan of fronds with leaflets, max ~85 px tall (15% of 600)
			for (int k = 0; k < 5; ++k) {
				float a = -PI * 0.5f + (k - 2) * 0.38f + sway;
				V2 dir(std::cos(a), std::sin(a));
				V2 base(sx, baseY);
				float L = 85.f * s;
				c.taper(base, base + dir * L, 5.f, 1.f, col);
				for (int q = 1; q < 7; ++q) {
					V2 p = base + dir * (L * q / 7.f);
					float ll = 18.f * s * (1.f - q / 8.f);
					c.leaf(p, norm(dir + perp(dir) * 0.9f), ll, ll * 0.3f, col2);
					c.leaf(p, norm(dir - perp(dir) * 0.9f), ll, ll * 0.3f, col2);
				}
			}
		} else if (f.kind == 1) {
			for (int k = 0; k < 2; ++k) {
				float a = -PI * 0.5f + (k ? 0.5f : -0.6f) + sway;
				c.leaf(V2(sx + (k ? 20.f : -20.f), baseY), V2(std::cos(a), std::sin(a)), 90.f * s, 28.f * s, col,
				       withAlpha(lerp(col, sf::Color::Black, 0.3f), 170));
			}
		} else {
			for (int k = 0; k < 9; ++k) {
				float a = -PI * 0.5f + (k - 4) * 0.12f + sway * 1.5f;
				V2 base(sx + (k - 4) * 6.f, baseY);
				c.taper(base, base + V2(std::cos(a), std::sin(a)) * (60.f * s * (0.7f + 0.3f * ((k * 7) % 3))), 7.f, 0.5f,
				        (k % 2) ? col : col2);
			}
		}
	}
	c.draw(t);
}
