#include "TerrainRenderer.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <random>

namespace {
const float T = 40.f;
const float PI = 3.14159265f;
const float R = 9.f; // rounded convex corner radius (px)
const sf::Color DRY(0xC9, 0xB3, 0x6B);

struct Rng {
	std::mt19937 g;
	explicit Rng(unsigned s) : g(s) {}
	float operator()(float a, float b) { return std::uniform_real_distribution<float>(a, b)(g); }
	int i(int a, int b) { return std::uniform_int_distribution<int>(a, b)(g); }
	bool chance(float p) { return (*this)(0.f, 1.f) < p; }
};
} // namespace

void TerrainRenderer::build(const LevelData& L, const Assets& assets, int seed) {
	myAssets = &assets;
	pal = paletteFor(L.biome);
	biome = L.biome;
	levelW = L.width * T;
	levelH = L.height * T;
	buildGeometry(L, seed);

	chunks.clear();
	const int CH = 32 * static_cast<int>(T);
	for (int cy = 0; cy < static_cast<int>(levelH); cy += CH) {
		for (int cx = 0; cx < static_cast<int>(levelW); cx += CH) {
			Chunk c;
			c.rt = std::make_unique<sf::RenderTexture>();
			int w = std::min(CH, static_cast<int>(levelW) - cx);
			int h = std::min(CH, static_cast<int>(levelH) - cy);
			if (!c.rt->create(w, h)) continue;
			c.pos = V2(static_cast<float>(cx), static_cast<float>(cy));
			sf::RenderTexture& rt = *c.rt;
			rt.setView(sf::View(sf::FloatRect(static_cast<float>(cx), static_cast<float>(cy), static_cast<float>(w),
			                                  static_cast<float>(h))));
			rt.clear(sf::Color::Transparent);
			fill.draw(rt);
			// textured earth: multiply a world-space tileable noise over the fill
			sf::VertexArray q(sf::Quads, 4);
			const float s = 0.55f;
			V2 p[4] = {V2(cx, cy), V2(cx + w, cy), V2(cx + w, cy + h), V2(cx, cy + h)};
			for (int i = 0; i < 4; ++i) {
				q[i].position = p[i];
				q[i].texCoords = p[i] * s;
				q[i].color = sf::Color::White;
			}
			sf::RenderStates st;
			st.texture = &assets.noise;
			st.blendMode = sf::BlendMultiply;
			rt.draw(q, st);
			detail.draw(rt);
			rt.display();
			rt.setSmooth(true);
			chunks.push_back(std::move(c));
		}
	}
	// geometry no longer needed once baked
	fill.clear();
	detail.clear();
}

void TerrainRenderer::buildGeometry(const LevelData& L, int seed) {
	Rng r(static_cast<unsigned>(seed) * 104729u + 7u);
	fill.clear();
	detail.clear();
	blades.clear();
	roots.clear();
	moss.clear();
	const int W = L.width, H = L.height;
	auto at = [&](int x, int y) -> char {
		if (y >= H) return '#';
		return L.at(x, y);
	};
	auto solid = [&](int x, int y) { return at(x, y) == '#'; };
	auto fillish = [&](int x, int y) {
		char c = at(x, y);
		return c == '#' || c == '/' || c == '\\';
	};
	auto wet = [&](int x, int y) { return L.waterAt(x, y) != W_NONE; };

	// distance-to-air field (tiles)
	std::vector<int> depth(W * H, 99);
	std::queue<std::pair<int, int>> q;
	for (int y = 0; y < H; ++y)
		for (int x = 0; x < W; ++x)
			if (!fillish(x, y)) {
				depth[y * W + x] = 0;
				q.push({x, y});
			}
	while (!q.empty()) {
		auto [x, y] = q.front();
		q.pop();
		const int dx[4] = {1, -1, 0, 0}, dy[4] = {0, 0, 1, -1};
		for (int k = 0; k < 4; ++k) {
			int nx = x + dx[k], ny = y + dy[k];
			if (nx < 0 || ny < 0 || nx >= W || ny >= H) continue;
			if (depth[ny * W + nx] > depth[y * W + x] + 1) {
				depth[ny * W + nx] = depth[y * W + x] + 1;
				q.push({nx, ny});
			}
		}
	}
	auto dAt = [&](int x, int y) -> float {
		if (x < 0 || x >= W) return 8.f;
		if (y >= H) return static_cast<float>(depth[(H - 1) * W + std::clamp(x, 0, W - 1)] + (y - H + 1));
		if (y < 0) return 0.f;
		return fillish(x, y) ? static_cast<float>(std::min(depth[y * W + x], 12)) : 0.f;
	};
	auto vDepth = [&](int vx, int vy) { // grid vertex
		return (dAt(vx - 1, vy - 1) + dAt(vx, vy - 1) + dAt(vx - 1, vy) + dAt(vx, vy)) * 0.25f;
	};
	sf::Color topLit = lerp(pal.earth, pal.grassLight, 0.16f);
	auto shade = [&](float d) {
		if (d < 1.f) return lerp(topLit, pal.earth, d);
		return lerp(pal.earth, pal.earthDark, std::min(1.f, (d - 1.f) / 6.f) * 0.65f);
	};

	const bool dry = biome == Biome::Gully || biome == Biome::Ridge || biome == Biome::Cliffs || biome == Biome::Mill;
	const bool hollow = biome == Biome::Hollow;
	const bool swamp = biome == Biome::Swamp;
	const float capH = dry ? 10.f : 14.f;
	sf::Color capCol = dry ? lerp(pal.grass, DRY, 0.25f) : (hollow ? lerp(pal.grass, pal.earthDark, 0.25f) : pal.grass);
	sf::Color capHi = dry ? lerp(pal.grassLight, DRY, 0.3f) : pal.grassLight;

	// ---- fill: rounded tiles with depth shading ----
	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			char c = L.at(x, y);
			float x0 = x * T, y0 = y * T, x1 = x0 + T, y1 = y0 + T;
			if (c == '/' || c == '\\') {
				sf::Color col = shade(0.4f);
				if (c == '/') fill.tri(V2(x0, y1), V2(x1, y1), V2(x1, y0), col);
				else fill.tri(V2(x0, y1), V2(x1, y1), V2(x0, y0), col);
				continue;
			}
			if (c != '#') continue;
			bool tl = !fillish(x - 1, y) && !fillish(x, y - 1) && !fillish(x - 1, y - 1);
			bool tr = !fillish(x + 1, y) && !fillish(x, y - 1) && !fillish(x + 1, y - 1);
			bool br = !fillish(x + 1, y) && !fillish(x, y + 1) && !fillish(x + 1, y + 1);
			bool bl = !fillish(x - 1, y) && !fillish(x, y + 1) && !fillish(x - 1, y + 1);
			std::vector<V2> pts;
			std::vector<sf::Color> cols;
			auto corner = [&](bool round, V2 pt, V2 ctr, float a0, int vx, int vy) {
				sf::Color col = shade(vDepth(vx, vy));
				if (!round) {
					pts.push_back(pt);
					cols.push_back(col);
					return;
				}
				for (int i = 0; i <= 4; ++i) {
					float a = a0 + PI * 0.5f * i / 4.f;
					pts.push_back(ctr + V2(std::cos(a), std::sin(a)) * R);
					cols.push_back(col);
				}
			};
			corner(tl, V2(x0, y0), V2(x0 + R, y0 + R), PI, x, y);
			corner(tr, V2(x1, y0), V2(x1 - R, y0 + R), PI * 1.5f, x + 1, y);
			corner(br, V2(x1, y1), V2(x1 - R, y1 - R), 0.f, x + 1, y + 1);
			corner(bl, V2(x0, y1), V2(x0 + R, y1 - R), PI * 0.5f, x, y + 1);
			V2 ctr(x0 + T * 0.5f, y0 + T * 0.5f);
			sf::Color cc = shade(dAt(x, y) * 0.9f);
			for (size_t i = 0; i < pts.size(); ++i) {
				size_t j = (i + 1) % pts.size();
				fill.tri(ctr, pts[i], pts[j], cc, cols[i], cols[j]);
			}
		}
	}

	// ---- detail: edges, AO, caps, speckles, branches, thorns ----
	sf::Color edgeLight = lerp(pal.earth, pal.grassLight, 0.35f);
	sf::Color dark = pal.earthDark;
	const float E = 7.f;
	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			char c = L.at(x, y);
			float x0 = x * T, y0 = y * T, x1 = x0 + T, y1 = y0 + T;
			if (c == '#') {
				bool tl = !fillish(x - 1, y) && !fillish(x, y - 1) && !fillish(x - 1, y - 1);
				bool tr = !fillish(x + 1, y) && !fillish(x, y - 1) && !fillish(x + 1, y - 1);
				bool br = !fillish(x + 1, y) && !fillish(x, y + 1) && !fillish(x + 1, y + 1);
				bool bl = !fillish(x - 1, y) && !fillish(x, y + 1) && !fillish(x - 1, y + 1);
				if (!fillish(x, y - 1))
					detail.rectV(x0 + (tl ? R : 0), y0, T - (tl ? R : 0) - (tr ? R : 0), E, withAlpha(edgeLight, 200),
					             withAlpha(edgeLight, 0));
				if (!fillish(x, y + 1)) {
					detail.rectV(x0 + (bl ? R : 0), y1 - E * 1.4f, T - (bl ? R : 0) - (br ? R : 0), E * 1.4f, withAlpha(dark, 0),
					             withAlpha(dark, 255));
					if (bl) detail.arc(V2(x0 + R, y1 - R), R - 1.5f, 3.f, PI * 0.5f, PI, withAlpha(dark, 200), 5);
					if (br) detail.arc(V2(x1 - R, y1 - R), R - 1.5f, 3.f, 0.f, PI * 0.5f, withAlpha(dark, 200), 5);
				}
				if (!fillish(x - 1, y))
					detail.rectH(x0, y0 + (tl ? R : 0), E, T - (tl ? R : 0) - (bl ? R : 0), withAlpha(dark, 120),
					             withAlpha(dark, 0));
				if (!fillish(x + 1, y))
					detail.rectH(x1 - E, y0 + (tr ? R : 0), E, T - (tr ? R : 0) - (br ? R : 0), withAlpha(dark, 0),
					             withAlpha(dark, 120));
				// speckles
				float d = dAt(x, y);
				if (r.chance(0.02f) && d >= 1.f)
					detail.ellipse(V2(x0 + r(10.f, 30.f), y0 + r(10.f, 30.f)), r(4.f, 7.f), r(2.5f, 4.f),
					               withAlpha(lerp(pal.earth, pal.fog, 0.3f), 150), r(-0.3f, 0.3f));
				if (d <= 1.f && r.chance(0.18f))
					detail.ellipse(V2(x0 + r(6.f, 34.f), y0 + r(14.f, 34.f)), r(2.f, 4.f), r(1.5f, 3.f),
					               withAlpha(dark, 170), r(0.f, 3.f));
				if (d > 3.f && r.chance(0.05f)) {
					// buried root / strata line
					float yy = y0 + r(8.f, 32.f);
					detail.line(V2(x0 + r(0.f, 10.f), yy), V2(x0 + r(25.f, 40.f), yy + r(-4.f, 4.f)), 2.f, withAlpha(dark, 90));
				}
			} else if (!fillish(x, y)) {
				// ambient occlusion in inner corners
				sf::Color ao = withAlpha(dark, 110), none = withAlpha(dark, 0);
				const float S = 16.f;
				if (fillish(x, y + 1) && fillish(x - 1, y)) detail.tri(V2(x0, y1), V2(x0 + S, y1), V2(x0, y1 - S), ao, none, none);
				if (fillish(x, y + 1) && fillish(x + 1, y)) detail.tri(V2(x1, y1), V2(x1 - S, y1), V2(x1, y1 - S), ao, none, none);
				if (fillish(x, y - 1) && fillish(x - 1, y)) detail.tri(V2(x0, y0), V2(x0 + S, y0), V2(x0, y0 + S), ao, none, none);
				if (fillish(x, y - 1) && fillish(x + 1, y)) detail.tri(V2(x1, y0), V2(x1 - S, y0), V2(x1, y0 + S), ao, none, none);
			}
		}
	}

	// grass / moss caps on top edges (runs)
	auto addBlades = [&](float xa, float xb, float yTop, float angBase) {
		int tiles = std::max(1, static_cast<int>((xb - xa) / T + 0.5f));
		for (int t = 0; t < tiles; ++t) {
			float tx0 = xa + t * T;
			if (hollow) {
				for (int k = 0; k < 2; ++k)
					if (r.chance(0.6f)) moss.push_back(V2(tx0 + r(4.f, 36.f), yTop + r(-2.f, 6.f)));
				continue;
			}
			int n = dry ? (r.chance(0.5f) ? 2 : 0) : (swamp ? r.i(2, 3) : r.i(4, 6));
			for (int k = 0; k < n; ++k) {
				Blade b;
				b.base = V2(tx0 + r(2.f, 38.f), yTop + 3.f);
				if (angBase != 0.f) b.base.y = yTop + (b.base.x - xa) * std::tan(angBase) + 3.f;
				b.reed = swamp;
				b.h = swamp ? r(26.f, 34.f) : (dry ? r(8.f, 14.f) : r(10.f, 20.f));
				b.lean = r(-0.44f, 0.44f) + angBase * 0.5f;
				b.w = swamp ? 3.f : 3.5f;
				b.col = dry ? DRY : ((k % 2) ? pal.grass : pal.grassLight);
				if (swamp) b.col = (k % 2) ? lerp(pal.grass, pal.earthDark, 0.2f) : pal.grass;
				blades.push_back(b);
			}
		}
	};
	for (int y = 0; y < H; ++y) {
		int x = 0;
		while (x < W) {
			if (!(solid(x, y) && !fillish(x, y - 1) && !wet(x, y - 1))) {
				++x;
				continue;
			}
			int xa = x;
			while (x < W && solid(x, y) && !fillish(x, y - 1) && !wet(x, y - 1)) ++x;
			int xb = x; // exclusive
			float left = xa * T - (fillish(xa - 1, y) ? 0.f : 4.f);
			float right = xb * T + (fillish(xb, y) ? 0.f : 4.f);
			float y0 = y * T;
			std::vector<V2> top, bot;
			int steps = static_cast<int>((right - left) / (T / 6.f)) + 1;
			for (int i = 0; i <= steps; ++i) {
				float xx = left + (right - left) * i / steps;
				top.push_back(V2(xx, y0 - 1.5f));
				bot.push_back(V2(xx, y0 + capH + 2.5f * std::sin((xx / T) * 3.f * 2.f * PI + y)));
			}
			for (int i = 0; i < steps; ++i) detail.quad(top[i], top[i + 1], bot[i + 1], bot[i], capCol);
			detail.rect(left, y0 - 1.5f, right - left, 3.5f, capHi);
			if (!fillish(xa - 1, y)) detail.circle(V2(left + 1.f, y0 + capH * 0.45f), capH * 0.55f, capCol, 10);
			if (!fillish(xb, y)) detail.circle(V2(right - 1.f, y0 + capH * 0.45f), capH * 0.55f, capCol, 10);
			// flowers / accents on lush biomes
			if (!dry && !hollow) {
				for (int t = xa; t < xb; ++t)
					if (r.chance(0.07f)) {
						V2 fp(t * T + r(8.f, 32.f), y0 - r(5.f, 9.f));
						detail.line(fp, fp + V2(0.f, 9.f), 1.5f, pal.grass);
						for (int k = 0; k < 5; ++k)
							detail.circle(fp + rot(V2(3.2f, 0.f), k * 2.f * PI / 5.f), 2.6f, pal.accent, 6);
						detail.circle(fp, 1.8f, sf::Color(255, 240, 180), 6);
					}
			}
			addBlades(xa * T, xb * T, y0, 0.f);
		}
	}
	// slope caps
	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			char c = L.at(x, y);
			if (c != '/' && c != '\\') continue;
			float x0 = x * T, y0 = y * T, x1 = x0 + T, y1 = y0 + T;
			V2 a = c == '/' ? V2(x0, y1) : V2(x0, y0);
			V2 b = c == '/' ? V2(x1, y0) : V2(x1, y1);
			V2 n = c == '/' ? V2(0.707f, 0.707f) : V2(-0.707f, 0.707f);
			detail.quad(a - n * 1.5f, b - n * 1.5f, b + n * capH, a + n * capH, capCol);
			detail.line(a - n * 1.f, b - n * 1.f, 3.f, capHi);
			addBlades(x0, x1, c == '/' ? y1 : y0, c == '/' ? -PI * 0.25f : PI * 0.25f);
		}
	}
	std::sort(blades.begin(), blades.end(), [](const Blade& a, const Blade& b) { return a.base.x < b.base.x; });

	// dangling roots
	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			if (!solid(x, y) || fillish(x, y + 1) || fillish(x, y + 2) || wet(x, y + 1) || y + 2 >= H) continue;
			if (!r.chance(0.3f)) continue;
			int n = r.i(1, 3);
			for (int k = 0; k < n; ++k) {
				V2 a(x * T + r(6.f, 34.f), (y + 1) * T - 2.f);
				float L2 = r(20.f, 60.f);
				V2 b = a + V2(r(-10.f, 10.f), L2);
				V2 c = (a + b) * 0.5f + V2(r(-12.f, 12.f), 0.f);
				roots.push_back({a, c, b, r(0.f, 6.f)});
			}
		}
	}
	std::sort(roots.begin(), roots.end(), [](const Root& a, const Root& b) { return a.a.x < b.a.x; });

	// one-way branches
	sf::Color bark = pal.earthDark;
	sf::Color barkLight = lerp(pal.earthDark, pal.grassLight, 0.3f);
	sf::Color barkDark = lerp(pal.earthDark, sf::Color::Black, 0.3f);
	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			if (L.at(x, y) != '=' || L.at(x - 1, y) == '=') continue;
			int n = 0;
			while (L.at(x + n, y) == '=') ++n;
			float xa = x * T, xb = (x + n) * T, y0 = y * T;
			// underside leaves first
			for (int t = 0; t < n; t += 2) {
				V2 base(xa + t * T + r(10.f, 30.f), y0 + 10.f);
				detail.leaf(base, norm(V2(-0.55f, 0.85f)), 16.f, 5.5f, pal.grass, lerp(pal.grass, pal.earthDark, 0.3f));
				detail.leaf(base, norm(V2(0.55f, 0.85f)), 14.f, 5.f, pal.grassLight, lerp(pal.grass, pal.earthDark, 0.3f));
			}
			// twigs at both ends
			detail.taper(V2(xa + 2.f, y0 + 6.f), V2(xa - 14.f, y0 - 6.f), 5.f, 1.5f, bark);
			detail.taper(V2(xa + 2.f, y0 + 8.f), V2(xa - 12.f, y0 + 14.f), 4.f, 1.5f, bark);
			detail.leaf(V2(xa - 13.f, y0 - 5.f), norm(V2(-0.6f, -0.8f)), 12.f, 4.f, pal.grass);
			detail.taper(V2(xb - 2.f, y0 + 6.f), V2(xb + 14.f, y0 - 6.f), 5.f, 1.5f, bark);
			detail.taper(V2(xb - 2.f, y0 + 8.f), V2(xb + 12.f, y0 + 14.f), 4.f, 1.5f, bark);
			detail.leaf(V2(xb + 13.f, y0 - 5.f), norm(V2(0.6f, -0.8f)), 12.f, 4.f, pal.grass);
			detail.roundRect(xa - 3.f, y0 - 1.f, xb - xa + 6.f, 13.f, 6.f, bark);
			detail.rect(xa + 2.f, y0 - 1.f, xb - xa - 4.f, 3.f, barkLight);
			detail.rect(xa + 2.f, y0 + 9.f, xb - xa - 4.f, 3.f, barkDark);
			for (int t = 0; t < n; ++t) {
				float nx = xa + t * T + r(12.f, 28.f);
				detail.line(V2(nx, y0 + 3.f), V2(nx + 4.f, y0 + 8.f), 1.5f, barkDark);
			}
		}
	}

	// thorns
	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			if (L.at(x, y) != '^') continue;
			int dir = 0;
			if (solid(x, y + 1) || L.at(x, y + 1) == '-') dir = 0;
			else if (solid(x, y - 1)) dir = 1;
			else if (solid(x + 1, y)) dir = 2;
			else if (solid(x - 1, y)) dir = 3;
			V2 c(x * T + T * 0.5f, y * T + T * 0.5f);
			V2 up = dir == 0 ? V2(0.f, -1.f) : dir == 1 ? V2(0.f, 1.f) : dir == 2 ? V2(-1.f, 0.f) : V2(1.f, 0.f);
			V2 side = perp(up);
			V2 base = c - up * (T * 0.5f);
			int n = r.i(5, 7);
			sf::Color stem(0x3B, 0x4A, 0x2A), stemB(0x5A, 0x42, 0x2A), tip(0xC9, 0x43, 0x3C);
			// bramble base
			detail.ellipse(base + up * 4.f, T * 0.52f, 7.f, stem, std::atan2(side.y, side.x));
			for (int k = 0; k < n; ++k) {
				float off = (k + 0.5f) / n * T - T * 0.5f + r(-3.f, 3.f);
				float h = r(14.f, 24.f);
				V2 d = norm(up + side * r(-0.35f, 0.35f));
				V2 b0 = base + side * off;
				V2 tp = b0 + d * h;
				V2 w = perp(d) * 4.5f;
				detail.tri(b0 + w, b0 - w, tp, (k % 2) ? stem : stemB);
				V2 tb = b0 + d * (h * 0.72f);
				detail.tri(tb + perp(d) * 1.6f, tb - perp(d) * 1.6f, tp, tip);
			}
		}
	}
}

void TerrainRenderer::drawStatic(sf::RenderTarget& t, const sf::FloatRect& view) const {
	for (const Chunk& c : chunks) {
		sf::FloatRect r(c.pos.x, c.pos.y, static_cast<float>(c.rt->getSize().x), static_cast<float>(c.rt->getSize().y));
		if (!r.intersects(view)) continue;
		sf::Sprite s(c.rt->getTexture());
		s.setPosition(c.pos);
		t.draw(s);
	}
}

void TerrainRenderer::drawDynamic(sf::RenderTarget& t, const sf::FloatRect& view, float time, float wind) const {
	Canvas c;
	float l = view.left - 60.f, rgt = view.left + view.width + 60.f;
	auto it = std::lower_bound(blades.begin(), blades.end(), l, [](const Blade& b, float x) { return b.base.x < x; });
	for (; it != blades.end() && it->base.x < rgt; ++it) {
		const Blade& b = *it;
		if (b.base.y < view.top - 60.f || b.base.y > view.top + view.height + 60.f) continue;
		float sway = std::sin(time * 2.f + b.base.x / T * 0.7f) * (b.reed ? 3.5f : 2.f) + wind * b.h * 0.25f;
		V2 tip = b.base + rot(V2(0.f, -b.h), b.lean) + V2(sway, 0.f);
		V2 n = perp(norm(tip - b.base)) * (b.w * 0.5f);
		c.tri(b.base + n, b.base - n, tip, b.col);
		if (b.reed && static_cast<int>(b.base.x) % 5 == 0) c.ellipse(tip + V2(0.f, 5.f), 2.5f, 6.f, pal.earthDark);
	}
	auto rt = std::lower_bound(roots.begin(), roots.end(), l, [](const Root& r, float x) { return r.a.x < x; });
	for (; rt != roots.end() && rt->a.x < rgt; ++rt) {
		if (rt->a.y < view.top - 80.f || rt->a.y > view.top + view.height + 20.f) continue;
		float sw = std::sin(time * 1.2f + rt->phase) * 2.f;
		auto pts = quadBezier(rt->a, rt->c + V2(sw * 0.5f, 0.f), rt->b + V2(sw, 0.f), 6);
		c.polyline(pts, 3.f, pal.earthDark, false);
		c.circle(pts.back(), 2.6f, pal.earthDark, 6);
	}
	c.draw(t);
	if (!moss.empty()) {
		SpriteBatch g;
		for (const V2& m : moss) {
			if (!view.contains(m)) continue;
			float p = 0.6f + 0.4f * std::sin(time * 1.7f + m.x * 0.13f);
			g.add(Tex::Glow, m, V2(10.f, 10.f), 0.f, withAlpha(pal.grassLight, static_cast<int>(90 * p)));
			g.add(Tex::Drop, m, V2(2.2f, 2.2f), 0.f, withAlpha(lerp(pal.grassLight, sf::Color::White, 0.5f), 230));
		}
		g.draw(t, myAssets->atlas, sf::BlendAdd);
	}
}
