// Leafwind -- drawing of level objects, decorations and collectibles (docs/DESIGN.md 5.4-5.5).
#include "Renderer.h"

#include "core/Config.h"

#include <algorithm>
#include <cmath>

using namespace cfg;

namespace {
const float T = TILE_PX;
const float PI = 3.14159265f;
inline V2 px(b2Vec2 m) { return V2(m.x * PPM, m.y * PPM); }

const sf::Color WOOD(0xB0, 0x7A, 0x45);
const sf::Color WOOD_DARK(0x7A, 0x50, 0x2A);
const sf::Color ROPE(0xC9, 0xA6, 0x6B);
const sf::Color VINE(0x3F, 0x6B, 0x2A);
const sf::Color VINE_CORE(0x7B, 0xB0, 0x5A);
const sf::Color IRON(0x5B, 0x5B, 0x5B);
const sf::Color LOG(0x8B, 0x5A, 0x2B);
const sf::Color CRATE(0xA9, 0x74, 0x3F);
const sf::Color ROCK(0x8C, 0x83, 0x78);
const sf::Color ROCK_DARK(0x6F, 0x67, 0x5D);
const sf::Color SLAB(0x8E, 0x8C, 0x86);

bool visible(const sf::FloatRect& r, V2 p, float margin) {
	return p.x > r.left - margin && p.x < r.left + r.width + margin && p.y > r.top - margin &&
	       p.y < r.top + r.height + margin;
}

void plankWithEdge(Canvas& c, V2 ctr, V2 half, float ang, sf::Color col, sf::Color edge) {
	c.roundRectRot(ctr, half + V2(1.5f, 1.5f), 3.f, ang, edge);
	c.roundRectRot(ctr, half, 2.5f, ang, col);
}

// Draws a sagging 2 px rope between two points.
void sagRope(Canvas& c, V2 a, V2 b, float sag, sf::Color col, float w = 2.f) {
	V2 mid = (a + b) * 0.5f + V2(0.f, sag);
	c.polyline(quadBezier(a, mid, b, 10), w, col, false);
}

void banana(Canvas& c, V2 p, float s, float ang) {
	// thick crescent from a quadratic curve with dark tips
	V2 a = p + rot(V2(-9.f, -5.f) * s, ang), b = p + rot(V2(9.f, -6.f) * s, ang), ctrl = p + rot(V2(0.f, 9.f) * s, ang);
	auto pts = quadBezier(a, ctrl, b, 10);
	for (size_t i = 0; i + 1 < pts.size(); ++i) {
		float t = static_cast<float>(i) / (pts.size() - 1);
		float w = (2.f + 5.f * std::sin(t * PI)) * s;
		c.taper(pts[i], pts[i + 1], w, (2.f + 5.f * std::sin((i + 1.f) / (pts.size() - 1) * PI)) * s, ui::Banana);
	}
	// highlight
	auto hl = quadBezier(a + V2(2.f, 1.f) * s, ctrl + V2(0.f, -3.f) * s, b + V2(-3.f, 1.f) * s, 8);
	for (size_t i = 2; i + 3 < hl.size(); ++i) c.line(hl[i], hl[i + 1], 1.6f * s, sf::Color(255, 244, 170));
	c.circle(a, 2.f * s, sf::Color(0x5A, 0x40, 0x10), 6);
	c.circle(b, 2.2f * s, sf::Color(0x5A, 0x40, 0x10), 6);
}

void figShape(Canvas& c, V2 p, float s, sf::Color body, sf::Color hi) {
	std::vector<V2> pts;
	for (int i = 0; i < 20; ++i) {
		float a = 2.f * PI * i / 20.f;
		float r = 9.f * s * (1.f + 0.35f * std::max(0.f, -std::sin(a)) * 0.9f);
		V2 q(std::cos(a) * r * (1.f - 0.25f * std::max(0.f, -std::sin(a))), std::sin(a) * r);
		pts.push_back(p + q + V2(0.f, 2.f * s));
	}
	c.polygon(pts, body);
	c.ellipse(p + V2(-3.f, -1.f) * s, 2.5f * s, 4.f * s, hi, 0.4f);
	c.rect(p.x - 1.f * s, p.y - 12.f * s, 2.f * s, 4.f * s, sf::Color(0x4A, 0x36, 0x20));
	c.leaf(p + V2(1.f, -11.f) * s, norm(V2(1.f, -0.4f)), 8.f * s, 3.f * s, sf::Color(0x58, 0xB3, 0x48));
}

} // namespace

// Golden leaf shape (concept art): used for the storm exit and story cards.
void goldenLeaf(Canvas& c, V2 ctr, float w, float h, float ang, sf::Color body, sf::Color vein) {
	V2 dir = rot(V2(0.f, -1.f), ang);
	V2 base = ctr - dir * (h * 0.5f);
	c.leaf(base, dir, h, w * 0.5f, body);
	c.taper(base - dir * 10.f, base + dir * (h * 0.9f), 4.f, 1.f, vein);
	for (int i = 1; i < 6; ++i) {
		V2 p = base + dir * (h * i / 7.f);
		float l = w * 0.38f * std::sin(PI * i / 6.5f);
		c.line(p, p + rot(dir, 0.9f) * l, 2.f, vein);
		c.line(p, p + rot(dir, -0.9f) * l, 2.f, vein);
	}
}

void Renderer::drawDecor(Canvas& c, SpriteBatch& glow, const World& w, float time) const {
	const sf::FloatRect vr = cam.rect();
	const LevelData& L = w.level;

	// ---- exit ----
	{
		V2 base(w.exitX * T + T * 0.5f, (w.exitY + 1) * T);
		if (visible(vr, base, 240.f)) {
			if (L.biome == Biome::Storm) {
				V2 lc = base - V2(0.f, T * 0.5f + T * 1.5f + 4.f * std::sin(time * 1.5f));
				for (int k = 0; k < 8; ++k) {
					float a = time * 0.4f + k * PI / 4.f;
					V2 d(std::cos(a), std::sin(a));
					c.tri(lc, lc + rot(d, 0.08f) * (T * 3.5f), lc + rot(d, -0.08f) * (T * 3.5f), withAlpha(ui::GoldCore, 40));
				}
				glow.add(Tex::Glow, lc, V2(T * 3.f, T * 3.f), 0.f, withAlpha(ui::GoldEdge, 120));
				goldenLeaf(c, lc, T * 2.f, T * 3.f, 0.25f * std::sin(time * 0.8f), ui::GoldCore, ui::GoldEdge);
			} else {
				// hollow tree: trunk, dark oval, warm glow, rim leaves
				sf::Color bark = lerp(pal.earthDark, pal.near, 0.3f);
				sf::Color barkHi = lerp(bark, pal.grassLight, 0.15f);
				c.quad(base + V2(-T * 1.9f, 0.f), base + V2(T * 1.9f, 0.f), base + V2(T * 1.3f, -T * 4.2f),
				       base + V2(-T * 1.3f, -T * 4.2f), bark);
				c.quad(base + V2(-T * 1.5f, 0.f), base + V2(-T * 0.9f, 0.f), base + V2(-T * 0.9f, -T * 4.2f),
				       base + V2(-T * 1.2f, -T * 4.2f), barkHi);
				c.tri(base + V2(-T * 2.6f, 0.f), base + V2(-T * 1.2f, -T * 1.2f), base + V2(-T * 1.2f, 0.f), bark);
				c.tri(base + V2(T * 2.6f, 0.f), base + V2(T * 1.2f, -T * 1.2f), base + V2(T * 1.2f, 0.f), bark);
				for (int k = 0; k < 7; ++k) {
					V2 p = base + V2(std::cos(k * 0.9f) * T * 1.4f, -T * 4.3f - std::sin(k * 1.7f) * T * 0.6f);
					c.circle(p, T * (0.9f + 0.2f * (k % 3)), lerp(pal.mid, pal.grass, 0.3f));
					c.circle(p + V2(-6.f, -8.f), T * (0.55f + 0.1f * (k % 3)), lerp(pal.grass, pal.grassLight, 0.3f));
				}
				V2 hc = base - V2(0.f, T * 1.25f);
				c.ellipse(hc, T * 1.05f, T * 1.3f, lerp(bark, sf::Color::Black, 0.3f));
				c.ellipse(hc + V2(0.f, 2.f), T * 0.95f, T * 1.22f, sf::Color(0x1F, 0x16, 0x0E));
				glow.add(Tex::Glow, hc + V2(0.f, T * 0.3f), V2(T * 1.1f, T * 1.1f), 0.f, sf::Color(0xFF, 0xB1, 0x3B, 70));
				glow.add(Tex::Glow, hc, V2(T * 2.6f, T * 2.6f), 0.f, sf::Color(0xFF, 0xB1, 0x3B, 30));
				for (int k = 0; k < 10; ++k) {
					float a = PI + k * PI / 9.f + 0.1f;
					V2 p = hc + V2(std::cos(a) * T * 1.05f, std::sin(a) * T * 1.3f);
					c.leaf(p, V2(std::cos(a), std::sin(a)), 13.f, 5.f, (k % 2) ? pal.grass : pal.grassLight);
				}
				// bananas drifting up out of the hollow
				for (int k = 0; k < 3; ++k) {
					float u = std::fmod(time * 0.25f + k / 3.f, 1.f);
					V2 p = hc + V2(std::sin(u * 6.f + k) * 10.f, T * 0.6f - u * T * 2.2f);
					banana(c, p, 0.6f, std::sin(time + k));
				}
			}
		}
	}

	// ---- checkpoints: carved totem + lantern ----
	for (size_t i = 0; i < w.checkpoints.size(); ++i) {
		const Checkpoint& cp = w.checkpoints[i];
		V2 base(cp.tx * T + T * 0.5f, (cp.ty + 1) * T);
		if (!visible(vr, base, 120.f)) continue;
		sf::Color wood = lerp(WOOD, pal.earthDark, 0.35f);
		sf::Color woodHi = lerp(WOOD, pal.grassLight, 0.1f);
		// stacked rounded blocks (1.5 tiles)
		c.roundRect(base.x - 13.f, base.y - 20.f, 26.f, 20.f, 5.f, wood);
		c.roundRect(base.x - 11.f, base.y - 42.f, 22.f, 22.f, 6.f, woodHi);
		c.roundRect(base.x - 14.f, base.y - 58.f, 28.f, 16.f, 5.f, wood);
		// carved face
		c.circle(V2(base.x - 5.f, base.y - 34.f), 2.5f, pal.earthDark, 6);
		c.circle(V2(base.x + 5.f, base.y - 34.f), 2.5f, pal.earthDark, 6);
		c.arc(V2(base.x, base.y - 29.f), 5.f, 2.f, 0.3f, PI - 0.3f, pal.earthDark, 6);
		c.line(V2(base.x - 12.f, base.y - 10.f), V2(base.x + 12.f, base.y - 10.f), 2.f, pal.earthDark);
		// lantern on a hook
		c.line(V2(base.x + 12.f, base.y - 54.f), V2(base.x + 22.f, base.y - 54.f), 3.f, pal.earthDark);
		V2 lp(base.x + 22.f, base.y - 44.f);
		c.line(V2(lp.x, lp.y - 10.f), V2(lp.x, lp.y - 6.f), 1.5f, pal.earthDark);
		c.roundRect(lp.x - 6.f, lp.y - 7.f, 12.f, 15.f, 3.f, sf::Color(0x3A, 0x2C, 0x1E));
		if (cp.lit) {
			float pulse = 1.f + 0.1f * std::sin(time * 4.f + i);
			c.roundRect(lp.x - 4.f, lp.y - 5.f, 8.f, 11.f, 2.f, ui::GoldCore);
			glow.add(Tex::Glow, lp, V2(1.5f * T, 1.5f * T) * pulse, 0.f, sf::Color(0xFF, 0xD9, 0x72, 90));
			glow.add(Tex::Glow, lp, V2(12.f, 12.f), 0.f, sf::Color(255, 240, 200, 160));
			for (int k = 0; k < 6; ++k) {
				float a = time * (0.8f + 0.1f * k) + k * PI / 3.f;
				V2 fp = lp + V2(std::cos(a) * (26.f + 6.f * std::sin(time + k)), std::sin(a * 1.3f) * 18.f - 4.f);
				glow.add(Tex::Glow, fp, V2(6.f, 6.f), 0.f, sf::Color(0xDF, 0xF4, 0x8A, 150));
				glow.add(Tex::Drop, fp, V2(1.5f, 1.5f), 0.f, sf::Color(0xF4, 0xFF, 0xC0, 255));
			}
		} else {
			c.roundRect(lp.x - 4.f, lp.y - 5.f, 8.f, 11.f, 2.f, sf::Color(0x5A, 0x4C, 0x3A));
		}
	}

	// ---- signposts ----
	for (const Sign& s : w.signs) {
		V2 base(s.tx * T + T * 0.5f, (s.ty + 1) * T);
		if (!visible(vr, base, 80.f)) continue;
		c.rect(base.x - 3.f, base.y - 34.f, 6.f, 34.f, WOOD_DARK);
		c.roundRect(base.x - 17.f, base.y - 38.f, 34.f, 20.f, 3.f, WOOD_DARK);
		c.roundRect(base.x - 15.f, base.y - 36.f, 30.f, 16.f, 2.f, WOOD);
		for (int k = 0; k < 3; ++k)
			c.line(V2(base.x - 10.f, base.y - 32.f + k * 4.5f), V2(base.x + 10.f - (k == 2 ? 8.f : 0.f), base.y - 32.f + k * 4.5f),
			       1.5f, WOOD_DARK);
	}

	// ---- bridge end posts ----
	for (const auto& br : w.bridges) {
		for (int side = 0; side < 2; ++side) {
			V2 a = px(side ? br->rightAnchor : br->leftAnchor);
			if (!visible(vr, a, 100.f)) continue;
			float dx = side ? 5.f : -5.f;
			c.roundRect(a.x + dx - 6.f, a.y - 30.f, 12.f, 34.f, 3.f, WOOD_DARK);
			c.circle(V2(a.x + dx, a.y - 30.f), 5.f, WOOD, 8);
		}
	}

	// ---- see-saw stumps (drawn only; no fixture) ----
	for (const auto& s : w.seesaws) {
		V2 p = px(s->pivot);
		if (!visible(vr, p, 200.f)) continue;
		int tx = static_cast<int>(s->pivot.x / TILE), ty = static_cast<int>(s->pivot.y / TILE) + 1;
		while (ty < L.height && !L.solid(tx, ty)) ++ty;
		float gy = ty * T;
		c.taper(V2(p.x, gy + 4.f), V2(p.x, p.y + 14.f), 18.f, 10.f, pal.earthDark);
		c.taper(V2(p.x, p.y + 16.f), V2(p.x - 10.f, p.y + 2.f), 7.f, 5.f, pal.earthDark);
		c.taper(V2(p.x, p.y + 16.f), V2(p.x + 10.f, p.y + 2.f), 7.f, 5.f, pal.earthDark);
		c.circle(p, 5.f, lerp(pal.earthDark, sf::Color::Black, 0.3f), 10);
	}

	// ---- hanging-log vines ----
	for (const auto& lg : w.fallLogs) {
		if (lg->state == FallLog::Falling || lg->state == FallLog::Resting) continue;
		V2 p = px(lg->body->GetPosition());
		float hw = lg->tiles * T * 0.5f;
		if (!visible(vr, p, hw + 200.f)) continue;
		for (int k = 0; k < 2; ++k) {
			float x = p.x + (k ? hw * 0.6f : -hw * 0.6f);
			int tx = static_cast<int>(x / T), ty = lg->y0 - 1;
			while (ty > 0 && !L.solid(tx, ty) && L.at(tx, ty) != '=') --ty;
			float top = std::max(vr.top - 20.f, (ty + 1) * T);
			float wob = lg->wobble * 60.f;
			V2 a(x, top), b(x + wob, p.y - T * 0.5f + 2.f);
			c.line(a, b, 3.f, VINE);
			for (float yy = a.y + 20.f; yy < b.y - 10.f; yy += 36.f)
				c.leaf(V2(x + wob * (yy - a.y) / (b.y - a.y), yy), norm(V2((static_cast<int>(yy) / 36) % 2 ? 1.f : -1.f, 0.5f)), 11.f,
				       4.f, pal.grass);
		}
	}

	// ---- pulley wheels, beam ropes ----
	for (const auto& l : w.lifts) {
		V2 wc = px(l->wheel);
		V2 top = px(l->body->GetPosition()) - V2(0.f, 0.2f * PPM);
		if (!visible(vr, wc, 300.f) && !visible(vr, top, 300.f)) continue;
		c.line(wc + V2(-10.f, 0.f), top + V2(-l->w * T * 0.25f, 0.f), 2.f, ROPE);
		c.line(wc + V2(10.f, 0.f), top + V2(l->w * T * 0.25f, 0.f), 2.f, ROPE);
		if (l->primary && l->partner) {
			V2 pw = px(l->partner->wheel);
			c.line(wc + V2(0.f, -11.f), pw + V2(0.f, -11.f), 2.f, ROPE);
		}
		c.line(wc, wc + V2(0.f, -18.f), 4.f, IRON);
		c.circle(wc, 12.f, WOOD_DARK, 16);
		c.circle(wc, 9.f, WOOD, 16);
		float a0 = l->body->GetPosition().y * 3.f;
		for (int k = 0; k < 6; ++k) c.line(wc, wc + rot(V2(9.f, 0.f), a0 + k * PI / 3.f), 2.f, WOOD_DARK);
		c.circle(wc, 3.f, IRON, 8);
	}
}

void Renderer::drawObjects(Canvas& c, SpriteBatch& glow, const World& w, float time) const {
	const sf::FloatRect vr = cam.rect();
	const LevelData& L = w.level;

	// ---- vines ----
	for (const auto& r : w.ropes) {
		V2 anchor = px(r->anchor);
		if (!visible(vr, anchor, r->lengthTiles * T + 100.f)) continue;
		// stub branch + knot
		c.roundRect(anchor.x - 20.f, anchor.y - 7.f, 40.f, 12.f, 6.f, pal.earthDark);
		c.rect(anchor.x - 16.f, anchor.y - 7.f, 32.f, 3.f, lerp(pal.earthDark, pal.grassLight, 0.3f));
		c.leaf(V2(anchor.x + 18.f, anchor.y - 2.f), norm(V2(1.f, -0.5f)), 12.f, 4.f, pal.grass);
		c.leaf(V2(anchor.x - 18.f, anchor.y - 2.f), norm(V2(-1.f, -0.3f)), 10.f, 4.f, pal.grassLight);
		std::vector<V2> pts{anchor};
		for (b2Body* s : r->segs) pts.push_back(px(s->GetPosition()));
		b2Body* last = r->segs.back();
		pts.push_back(px(last->GetWorldPoint(b2Vec2(0.f, ROPE_SEG_LEN * 0.5f))));
		auto curve = catmullRom(pts, 3);
		c.polyline(curve, 5.f, VINE);
		c.polyline(curve, 2.f, VINE_CORE, false);
		for (size_t i = 2; i < r->segs.size(); i += 3) {
			V2 p = px(r->segs[i]->GetPosition());
			V2 d = norm(rot(V2(0.f, 1.f), r->segs[i]->GetAngle()));
			float side = (i / 3) % 2 ? 1.f : -1.f;
			c.leaf(p, norm(d + perp(d) * side * 1.4f), 12.f, 4.5f, pal.grass, lerp(pal.grass, VINE, 0.5f));
			c.leaf(p + d * 3.f, norm(d - perp(d) * side * 1.2f), 10.f, 4.f, pal.grassLight);
		}
		c.circle(anchor, 5.f, lerp(VINE, pal.earthDark, 0.5f), 10);
		c.circle(curve.back(), 3.5f, VINE, 8);
	}

	// ---- rope bridges ----
	for (const auto& br : w.bridges) {
		V2 la = px(br->leftAnchor), ra = px(br->rightAnchor);
		if (!visible(vr, (la + ra) * 0.5f, (ra.x - la.x) * 0.5f + 100.f)) continue;
		std::vector<V2> low{la}, rail{la + V2(-5.f, -26.f)};
		for (b2Body* p : br->planks) {
			V2 cpos = px(p->GetPosition());
			low.push_back(cpos + rot(V2(0.f, 1.f), p->GetAngle()));
			rail.push_back(cpos + V2(0.f, -24.f));
		}
		low.push_back(ra);
		rail.push_back(ra + V2(5.f, -26.f));
		auto railC = catmullRom(rail, 3);
		c.polyline(railC, 2.f, ROPE, false);
		for (size_t i = 1; i + 1 < rail.size(); ++i) c.line(rail[i], low[i], 1.5f, withAlpha(ROPE, 200));
		for (b2Body* p : br->planks) {
			V2 cpos = px(p->GetPosition());
			plankWithEdge(c, cpos, V2(0.23f * PPM, 0.05f * PPM), p->GetAngle(), WOOD, WOOD_DARK);
			c.line(cpos + rot(V2(-14.f, -1.f), p->GetAngle()), cpos + rot(V2(10.f, -1.f), p->GetAngle()), 1.f,
			       lerp(WOOD, sf::Color::White, 0.2f));
		}
		c.polyline(catmullRom(low, 3), 2.f, ROPE, false);
	}

	// ---- lift platforms ----
	for (const auto& l : w.lifts) {
		V2 p = px(l->body->GetPosition());
		float hw = l->w * T * 0.5f - 1.6f;
		if (!visible(vr, p, hw + 40.f)) continue;
		c.roundRect(p.x - hw, p.y - 16.f, hw * 2.f, 32.f, 4.f, WOOD_DARK);
		for (int k = 0; k < 3; ++k) c.rect(p.x - hw + 2.f, p.y - 14.f + k * 10.f, hw * 2.f - 4.f, 8.f, WOOD);
		for (int k = 0; k < 2; ++k) {
			float bx = p.x + (k ? hw * 0.55f : -hw * 0.55f);
			c.rect(bx - 4.f, p.y - 16.f, 8.f, 32.f, IRON);
			c.circle(V2(bx, p.y - 10.f), 1.6f, sf::Color(0xAA, 0xAA, 0xAA), 6);
			c.circle(V2(bx, p.y + 10.f), 1.6f, sf::Color(0xAA, 0xAA, 0xAA), 6);
		}
	}

	// ---- movers ----
	for (const auto& m : w.movers) {
		V2 p = px(m->body->GetPosition());
		float hw = m->w * T * 0.5f;
		if (!visible(vr, p, hw + 60.f)) continue;
		if (m->lilyPad) {
			bool overWater = L.waterAt(static_cast<int>(p.x / T), static_cast<int>((p.y + 14.f) / T)) != W_NONE;
			if (overWater) c.ellipse(p + V2(0.f, 8.f), hw + 6.f, 5.f, withAlpha(sf::Color(0xDC, 0xF5, 0xFF), 70));
			else c.ellipse(p + V2(0.f, 26.f), hw * 0.8f, 4.f, sf::Color(0, 0, 0, 40));
			std::vector<V2> pad;
			const int N = 28;
			for (int i = 0; i <= N; ++i) {
				float a = 0.35f + (2.f * PI - 0.7f) * i / N - PI * 0.5f;
				pad.push_back(p + V2(std::cos(a) * hw, std::sin(a) * 10.f));
			}
			for (size_t i = 0; i + 1 < pad.size(); ++i) c.tri(p, pad[i], pad[i + 1], sf::Color(0x8E, 0xD4, 0x6A));
			for (size_t i = 0; i + 1 < pad.size(); ++i)
				c.tri(p + V2(0.f, -1.f), p + (pad[i] - p) * 0.9f + V2(0.f, -1.f), p + (pad[i + 1] - p) * 0.9f + V2(0.f, -1.f),
				      sf::Color(0x4F, 0x9A, 0x3C));
			for (int k = 0; k < 5; ++k)
				c.line(p, p + V2(std::cos(-PI * 0.5f + 0.9f + k * 1.1f) * hw * 0.8f, std::sin(-PI * 0.5f + 0.9f + k * 1.1f) * 8.f),
				       1.f, sf::Color(0x6F, 0xB8, 0x55));
			if (m->flower) {
				V2 f = p + V2(hw * 0.4f, -6.f);
				for (int k = 0; k < 6; ++k) c.leaf(f, rot(V2(0.f, -1.f), k * PI / 3.f), 9.f, 4.f, sf::Color(0xF0, 0x8C, 0xB4));
				c.circle(f, 3.f, sf::Color(0xFF, 0xE0, 0x70), 8);
			}
		} else {
			c.ellipse(p + V2(0.f, 26.f), hw * 0.8f, 4.f, sf::Color(0, 0, 0, 40));
			for (int k = -1; k <= 1; ++k)
				c.leaf(p + V2(-hw + 4.f + k * 6.f, 2.f * k), norm(V2(1.f, -0.05f * k)), hw * 2.f - 8.f, 9.f,
				       k == 0 ? pal.grassLight : pal.grass, lerp(pal.grass, pal.earthDark, 0.4f));
		}
	}

	// ---- see-saws ----
	for (const auto& s : w.seesaws) {
		V2 p = px(s->body->GetPosition());
		float hw = s->w * T * 0.5f;
		if (!visible(vr, p, hw + 60.f)) continue;
		float a = s->body->GetAngle();
		plankWithEdge(c, p, V2(hw, 6.f), a, WOOD, WOOD_DARK);
		for (int k = 0; k < 3; ++k)
			c.line(p + rot(V2(-hw + 10.f + k * 30.f, -2.f + k * 1.5f), a), p + rot(V2(-hw + 60.f + k * 45.f, -2.f + k * 1.5f), a),
			       1.f, lerp(WOOD, WOOD_DARK, 0.5f));
		if (s->lips)
			for (int side = -1; side <= 1; side += 2)
				c.rotRect(p + rot(V2(side * (hw - 3.2f), -10.4f), a), V2(3.2f, 4.8f), a, WOOD_DARK);
		c.circle(p, 4.f, IRON, 8);
	}

	// ---- floating & falling logs ----
	auto drawLog = [&](b2Body* b, int tiles, float wob, float alpha) {
		V2 p = px(b->GetPosition()) + V2(wob * 60.f, 0.f);
		float hw = tiles * T * 0.5f;
		if (!visible(vr, p, hw + 40.f)) return;
		float a = b->GetAngle();
		sf::Uint8 al = static_cast<sf::Uint8>(255 * alpha);
		c.roundRectRot(p, V2(hw, 19.f), 16.f, a, withAlpha(lerp(LOG, sf::Color::Black, 0.3f), al));
		c.roundRectRot(p, V2(hw - 1.5f, 17.f), 15.f, a, withAlpha(LOG, al));
		for (int k = 0; k < tiles; ++k) {
			float x = -hw + 14.f + k * (hw * 2.f - 28.f) / std::max(1, tiles - 1) * (tiles > 1 ? 1.f : 0.f);
			c.line(p + rot(V2(x, -6.f), a), p + rot(V2(x + 22.f, -5.f), a), 1.5f, withAlpha(lerp(LOG, sf::Color::Black, 0.25f), al));
			c.line(p + rot(V2(x + 6.f, 5.f), a), p + rot(V2(x + 26.f, 6.f), a), 1.5f, withAlpha(lerp(LOG, sf::Color::Black, 0.25f), al));
		}
		// end rings
		for (int side = -1; side <= 1; side += 2) {
			V2 e = p + rot(V2(side * (hw - 6.f), 0.f), a);
			c.ellipse(e, 6.f, 15.f, withAlpha(sf::Color(0xC8, 0x96, 0x5A), al), a);
			c.ellipse(e, 3.5f, 9.f, withAlpha(sf::Color(0xA0, 0x70, 0x40), al), a);
			c.ellipse(e, 1.5f, 4.f, withAlpha(sf::Color(0x8B, 0x5A, 0x2B), al), a);
		}
		// moss on top
		for (int k = 0; k < tiles * 2; ++k) {
			float x = -hw + 12.f + k * (hw * 2.f - 24.f) / std::max(1, tiles * 2 - 1);
			c.circle(p + rot(V2(x, -16.f), a), 5.f + 2.f * std::sin(k * 2.3f), withAlpha(pal.grass, al), 8);
		}
	};
	for (const auto& l : w.floatLogs)
		if (l->alive) drawLog(l->body, l->tiles, 0.f, 1.f);
	for (const auto& l : w.fallLogs)
		if (l->body->IsEnabled()) drawLog(l->body, l->tiles, l->wobble, 1.f);

	// ---- crates ----
	for (const auto& cr : w.crates) {
		if (!cr->alive) continue;
		V2 p = px(cr->body->GetPosition());
		if (!visible(vr, p, 40.f)) continue;
		float a = cr->body->GetAngle();
		float h = 0.24f * PPM;
		c.roundRectRot(p, V2(h + 2.f, h + 2.f), 5.f, a, pal.earthDark);
		c.roundRectRot(p, V2(h, h), 4.f, a, CRATE);
		for (int k = -1; k <= 1; ++k)
			c.line(p + rot(V2(-h + 2.f, k * 7.f), a), p + rot(V2(h - 2.f, k * 7.f), a), 1.5f, lerp(CRATE, pal.earthDark, 0.45f));
		sf::Color strap = lerp(CRATE, pal.earthDark, 0.7f);
		c.line(p + rot(V2(-h + 3.f, -h + 3.f), a), p + rot(V2(h - 3.f, h - 3.f), a), 4.f, strap);
		c.line(p + rot(V2(h - 3.f, -h + 3.f), a), p + rot(V2(-h + 3.f, h - 3.f), a), 4.f, strap);
		c.circle(p, 3.f, IRON, 8);
	}

	// ---- boulders ----
	for (const auto& b : w.boulders) {
		if (!b->alive) continue;
		V2 p = px(b->body->GetPosition());
		if (!visible(vr, p, 60.f)) continue;
		float a = b->body->GetAngle();
		float r = 0.475f * PPM;
		c.circle(p + V2(0.f, 2.f), r + 1.5f, lerp(ROCK_DARK, sf::Color::Black, 0.3f), 28);
		c.circle(p, r, ROCK, 28);
		const float fa[3] = {0.4f, 2.5f, 4.3f};
		for (int k = 0; k < 3; ++k) {
			float aa = a + fa[k];
			std::vector<V2> f = {p + rot(V2(r * 0.15f, 0.f), aa), p + rot(V2(r * 0.95f, -r * 0.3f), aa),
			                     p + rot(V2(r * 0.97f, r * 0.25f), aa), p + rot(V2(r * 0.55f, r * 0.45f), aa)};
			c.polygon(f, ROCK_DARK);
		}
		c.ellipse(p + rot(V2(-r * 0.2f, -r * 0.65f), a), r * 0.45f, r * 0.22f, pal.grass, a);
		c.ellipse(p + rot(V2(-r * 0.25f, -r * 0.7f), a), r * 0.25f, r * 0.1f, pal.grassLight, a);
		c.circle(p + V2(-r * 0.35f, -r * 0.35f), r * 0.2f, withAlpha(sf::Color::White, 40), 12);
	}

	// ---- crumbling ledges ----
	for (const auto& cr : w.crumbles) {
		if (cr->state == Crumble::Gone) continue;
		V2 p = px(cr->body->GetPosition());
		if (!visible(vr, p, 40.f)) continue;
		float a = cr->body->GetAngle();
		if (cr->state == Crumble::Shaking) p.x += std::sin(cr->timer * 20.f * 2.f * PI) * 2.f;
		sf::Uint8 al = static_cast<sf::Uint8>(255 * std::clamp(cr->alpha, 0.f, 1.f));
		c.roundRectRot(p, V2(20.f, 20.f), 5.f, a, withAlpha(lerp(SLAB, sf::Color::Black, 0.35f), al));
		c.roundRectRot(p + rot(V2(0.f, -1.f), a), V2(18.5f, 18.f), 4.f, a, withAlpha(SLAB, al));
		sf::Color crack = withAlpha(lerp(SLAB, sf::Color::Black, 0.5f), al);
		int seed = cr->tx * 7 + cr->ty * 13;
		std::vector<V2> k1 = {V2(-12.f, -18.f), V2(-6.f, -6.f), V2(-9.f, 4.f), V2(-2.f, 17.f)};
		std::vector<V2> k2 = {V2(18.f, -4.f), V2(7.f, 2.f), V2(10.f, 12.f)};
		for (auto& q : k1) q = p + rot(q + V2(static_cast<float>(seed % 5), 0.f), a);
		for (auto& q : k2) q = p + rot(q, a);
		c.polyline(k1, 1.5f, crack, false);
		c.polyline(k2, 1.5f, crack, false);
		c.ellipse(p + rot(V2(-13.f, -17.f), a), 8.f, 4.f, withAlpha(pal.grass, al), a);
		c.ellipse(p + rot(V2(12.f, -18.f), a), 7.f, 3.5f, withAlpha(pal.grassLight, al), a);
	}

	// ---- mushrooms ----
	bool glowCaps = L.biome == Biome::Hollow;
	for (const auto& m : w.mushrooms) {
		V2 base(m->tx * T + T * 0.5f, (m->ty + 1) * T);
		if (!visible(vr, base, 60.f)) continue;
		float s = m->squash;
		float sx = 1.f + 0.3f * s, sy = 1.f - 0.4f * s;
		sf::Color stem(0xF1, 0xE3, 0xC8);
		c.roundRect(base.x - 8.f, base.y - 20.f * sy - 2.f, 16.f, 20.f * sy + 2.f, 4.f, stem);
		c.rect(base.x - 8.f, base.y - 6.f, 16.f, 6.f, lerp(stem, pal.earthDark, 0.2f));
		sf::Color capC = glowCaps ? sf::Color(0x3F, 0xB0, 0xA0) : sf::Color(0xD8, 0x4C, 0x3F);
		V2 cc(base.x, base.y - 20.f * sy);
		float rx = 20.f * sx + 2.f, ry = 20.f * sy;
		std::vector<V2> cap{cc + V2(rx, 2.f)};
		for (int i = 0; i <= 16; ++i) {
			float aa = PI * i / 16.f;
			cap.push_back(cc + V2(std::cos(aa) * rx, -std::sin(aa) * ry));
		}
		cap.push_back(cc + V2(-rx, 2.f));
		c.polygon(cap, capC);
		c.rect(cc.x - rx, cc.y - 1.f, rx * 2.f, 4.f, lerp(capC, sf::Color::Black, 0.25f));
		sf::Color dot(0xFF, 0xF4, 0xD6);
		c.ellipse(cc + V2(-rx * 0.45f, -ry * 0.45f), 3.5f * sx, 3.f * sy, dot);
		c.ellipse(cc + V2(rx * 0.1f, -ry * 0.75f), 4.f * sx, 3.f * sy, dot);
		c.ellipse(cc + V2(rx * 0.55f, -ry * 0.35f), 3.f * sx, 2.5f * sy, dot);
		if (glowCaps)
			glow.add(Tex::Glow, cc + V2(0.f, -ry * 0.4f), V2(34.f, 26.f), 0.f,
			         sf::Color(0x3F, 0xE0, 0xC0, static_cast<sf::Uint8>(60 + 20 * std::sin(time * 2.f + m->tx))));
	}

	// ---- beetles ----
	for (const auto& b : w.beetles) {
		if (!b->body || !b->body->IsEnabled()) continue;
		V2 p = px(b->body->GetPosition());
		if (!visible(vr, p, 40.f)) continue;
		float alpha = b->dead ? std::max(0.f, 1.f - b->deadTimer / 1.2f) : 1.f;
		sf::Uint8 al = static_cast<sf::Uint8>(255 * alpha);
		float flip = b->dead ? -1.f : 1.f;
		float dir = static_cast<float>(b->dir);
		sf::Color leg = withAlpha(sf::Color(0x1A, 0x1A, 0x1A), al);
		for (int k = 0; k < 4; ++k) {
			float ph = b->walkPhase * 6.f * 2.f * PI + k * PI;
			float lx = -10.f + k * 6.5f;
			V2 a0 = p + V2(lx, 3.f * flip);
			V2 a1 = p + V2(lx + std::sin(ph) * 3.f, 10.f * flip);
			c.line(a0, a1, 2.f, leg);
		}
		// antennae
		V2 head = p + V2(dir * 13.f, 1.f * flip);
		c.line(head, head + V2(dir * 7.f, -7.f * flip), 1.5f, leg);
		c.line(head, head + V2(dir * 3.f, -9.f * flip), 1.5f, leg);
		c.circle(head, 4.f, withAlpha(sf::Color(0x1A, 0x22, 0x3A), al), 10);
		c.ellipse(p + V2(0.f, -1.f * flip), 15.f, 9.f, withAlpha(sf::Color(0x26, 0x36, 0x5C), al));
		c.line(p + V2(-dir * 2.f, -9.f * flip), p + V2(-dir * 2.f, 6.f * flip), 1.2f, withAlpha(sf::Color(0x1A, 0x22, 0x3A), al));
		c.ellipse(p + V2(-dir * 4.f, -5.f * flip), 6.f, 2.5f, withAlpha(sf::Color(0x5F, 0x79, 0xB5), al), -0.3f * dir);
		c.circle(p + V2(dir * 5.f, -5.f * flip), 1.5f, withAlpha(sf::Color(0xC8, 0xD6, 0xFF), al), 6);
	}
}

void Renderer::drawItems(Canvas& c, SpriteBatch& glow, const World& w, float time) const {
	const sf::FloatRect vr = cam.rect();
	for (const Collectible& it : w.items) {
		if (it.collected) continue;
		V2 p(it.x * PPM, it.y * PPM);
		if (!visible(vr, p, 40.f)) continue;
		float bob = 3.f * std::sin(time * 2.f * PI / 1.2f + it.x * 1.7f);
		p.y += bob;
		if (it.fig) {
			float pulse = 1.f + 0.15f * std::sin(time * 3.f);
			glow.add(Tex::Glow, p, V2(34.f, 34.f) * pulse, 0.f, sf::Color(0xFF, 0xD9, 0x72, 120));
			glow.add(Tex::Glow, p, V2(16.f, 16.f), 0.f, sf::Color(0xFF, 0xB1, 0x3B, 110));
			figShape(c, p, 1.25f, sf::Color(0x7A, 0x3E, 0x8C), sf::Color(0xB0, 0x7A, 0xC4));
			if (std::fmod(time, 1.1f) < 0.08f) glow.add(Tex::Star, p + V2(9.f, -9.f), V2(7.f, 7.f), time, ui::GoldCore);
		} else {
			glow.add(Tex::Glow, p, V2(14.f, 14.f), 0.f, sf::Color(0xFF, 0xE6, 0x80, 40));
			banana(c, p, 1.f, 0.35f * std::sin(time * 1.5f + it.x));
		}
	}
}
