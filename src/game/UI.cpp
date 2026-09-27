#include "UI.h"

#include <cmath>
#include <cstdio>
#include <sstream>

namespace uidraw {

static const float PI = 3.14159265f;

sf::String utf8(const std::string& s) { return sf::String::fromUtf8(s.begin(), s.end()); }

float textWidth(const sf::Font& f, const std::string& s, unsigned size) {
	sf::Text t(utf8(s), f, size);
	return t.getLocalBounds().width;
}

std::vector<std::string> wrap(const sf::Font& f, const std::string& s, unsigned size, float maxWidth) {
	std::vector<std::string> lines;
	std::istringstream in(s);
	std::string word, cur;
	while (in >> word) {
		std::string trial = cur.empty() ? word : cur + " " + word;
		if (!cur.empty() && textWidth(f, trial, size) > maxWidth) {
			lines.push_back(cur);
			cur = word;
		} else {
			cur = trial;
		}
	}
	if (!cur.empty()) lines.push_back(cur);
	return lines;
}

void text(sf::RenderTarget& t, const sf::Font& f, const std::string& s, unsigned size, V2 pos, sf::Color col, Align a,
          float alpha) {
	sf::Text tx(utf8(s), f, size);
	col.a = static_cast<sf::Uint8>(col.a * std::clamp(alpha, 0.f, 1.f));
	tx.setFillColor(col);
	sf::FloatRect b = tx.getLocalBounds();
	float x = pos.x;
	if (a == Center) x -= b.width * 0.5f + b.left;
	else if (a == Right) x -= b.width + b.left;
	tx.setPosition(std::round(x), std::round(pos.y));
	t.draw(tx);
}

void textShadow(sf::RenderTarget& t, const sf::Font& f, const std::string& s, unsigned size, V2 pos, sf::Color col,
                sf::Color shadow, V2 off, Align a, float alpha) {
	text(t, f, s, size, pos + off, shadow, a, alpha);
	text(t, f, s, size, pos, col, a, alpha);
}

void bananaIcon(Canvas& c, V2 p, float s, float alpha) {
	sf::Color y = withAlpha(ui::Banana, static_cast<int>(255 * alpha));
	auto pts = quadBezier(p + V2(-9.f, -6.f) * s, p + V2(0.f, 10.f) * s, p + V2(9.f, -7.f) * s, 10);
	for (size_t i = 0; i + 1 < pts.size(); ++i) {
		float t0 = static_cast<float>(i) / (pts.size() - 1), t1 = (i + 1.f) / (pts.size() - 1);
		c.taper(pts[i], pts[i + 1], (2.f + 5.f * std::sin(t0 * PI)) * s, (2.f + 5.f * std::sin(t1 * PI)) * s, y);
	}
	sf::Color tip = withAlpha(sf::Color(0x5A, 0x40, 0x10), static_cast<int>(255 * alpha));
	c.circle(pts.front(), 2.f * s, tip, 6);
	c.circle(pts.back(), 2.f * s, tip, 6);
}

void figIcon(Canvas& c, V2 p, float s, bool gold, float alpha) {
	int a = static_cast<int>(255 * alpha);
	sf::Color body = gold ? sf::Color(0xFF, 0xC8, 0x3C) : sf::Color(0x9A, 0x92, 0x88);
	sf::Color rim = gold ? ui::GoldEdge : sf::Color(0x6A, 0x62, 0x58);
	std::vector<V2> pts;
	for (int i = 0; i < 20; ++i) {
		float ang = 2.f * PI * i / 20.f;
		float r = 9.f * s * (1.f + 0.3f * std::max(0.f, -std::sin(ang)));
		V2 q(std::cos(ang) * r * (1.f - 0.25f * std::max(0.f, -std::sin(ang))), std::sin(ang) * r);
		pts.push_back(p + q + V2(0.f, 2.f * s));
	}
	std::vector<V2> outer;
	for (const V2& q : pts) outer.push_back(p + (q - p) * 1.15f + V2(0.f, -0.3f * s));
	c.polygon(outer, withAlpha(rim, a));
	c.polygon(pts, withAlpha(body, a));
	if (gold) c.ellipse(p + V2(-3.f, -1.f) * s, 2.3f * s, 3.8f * s, withAlpha(sf::Color(255, 244, 200), a), 0.4f);
	c.rect(p.x - 1.f * s, p.y - 12.f * s, 2.f * s, 4.f * s, withAlpha(sf::Color(0x4A, 0x36, 0x20), a));
}

void skullLeafIcon(Canvas& c, V2 p, float s, float alpha) {
	int a = static_cast<int>(255 * alpha);
	c.leaf(p + V2(-9.f, 7.f) * s, norm(V2(1.f, -0.8f)), 22.f * s, 7.f * s, withAlpha(sf::Color(0x7A, 0x6A, 0x50), a));
	c.circle(p + V2(0.f, -1.f) * s, 5.5f * s, withAlpha(ui::Cream, a), 12);
	c.rect(p.x - 3.f * s, p.y + 3.f * s, 6.f * s, 3.f * s, withAlpha(ui::Cream, a));
	c.circle(p + V2(-2.f, -1.f) * s, 1.4f * s, withAlpha(ui::Ink, a), 6);
	c.circle(p + V2(2.f, -1.f) * s, 1.4f * s, withAlpha(ui::Ink, a), 6);
}

void vineKnot(Canvas& c, V2 p, float s, sf::Color col) {
	c.ring(p, 9.f * s, 4.f * s, col, 18);
	c.line(p + V2(-16.f, -10.f) * s, p + V2(16.f, 10.f) * s, 4.f * s, col);
	c.line(p + V2(-16.f, 10.f) * s, p + V2(16.f, -10.f) * s, 4.f * s, col);
	c.leaf(p + V2(12.f, -8.f) * s, norm(V2(1.f, -0.6f)), 12.f * s, 4.f * s, col);
}

void swirl(Canvas& c, V2 p, float s, sf::Color col) {
	for (int k = 0; k < 3; ++k) {
		std::vector<V2> pts;
		for (int i = 0; i <= 40; ++i) {
			float t = i / 40.f;
			float a = t * 4.2f + k * 2.1f;
			float r = (6.f + 34.f * t) * s;
			pts.push_back(p + V2(std::cos(a) * r * 1.4f, std::sin(a) * r * 0.7f) + V2(k * 18.f - 18.f, k * 8.f - 8.f) * s);
		}
		c.polyline(pts, 3.f * s, col, true);
	}
	c.leaf(p + V2(46.f, -18.f) * s, norm(V2(1.f, -0.3f)), 20.f * s, 7.f * s, col);
	c.leaf(p + V2(-50.f, 16.f) * s, norm(V2(-1.f, 0.4f)), 16.f * s, 6.f * s, col);
}

void bigLeaf(Canvas& c, V2 p, float w, float h, float ang, sf::Color body, sf::Color vein) {
	goldenLeaf(c, p, w, h, ang, body, vein);
}

std::string formatTime(float seconds) {
	int s = static_cast<int>(seconds);
	char buf[32];
	std::snprintf(buf, sizeof(buf), "%d:%02d", s / 60, s % 60);
	return buf;
}

} // namespace uidraw
