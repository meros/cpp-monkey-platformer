#include "Draw.h"

#include <algorithm>
#include <cmath>

static const float PI = 3.14159265f;

float len(V2 a) { return std::sqrt(a.x * a.x + a.y * a.y); }
V2 norm(V2 a) {
	float l = len(a);
	return l > 1e-6f ? V2(a.x / l, a.y / l) : V2(0.f, 0.f);
}
V2 rot(V2 a, float ang) {
	float c = std::cos(ang), s = std::sin(ang);
	return V2(a.x * c - a.y * s, a.x * s + a.y * c);
}
V2 perp(V2 a) { return V2(-a.y, a.x); }

static int autoSegs(float r) { return std::clamp(static_cast<int>(r * 0.9f) + 8, 8, 48); }

void Canvas::tri(V2 a, V2 b, V2 c, sf::Color col) {
	va.append(sf::Vertex(a, col));
	va.append(sf::Vertex(b, col));
	va.append(sf::Vertex(c, col));
}
void Canvas::tri(V2 a, V2 b, V2 c, sf::Color ca, sf::Color cb, sf::Color cc) {
	va.append(sf::Vertex(a, ca));
	va.append(sf::Vertex(b, cb));
	va.append(sf::Vertex(c, cc));
}
void Canvas::quad(V2 a, V2 b, V2 c, V2 d, sf::Color col) {
	tri(a, b, c, col);
	tri(a, c, d, col);
}
void Canvas::quad(V2 a, V2 b, V2 c, V2 d, sf::Color ca, sf::Color cb, sf::Color cc, sf::Color cd) {
	tri(a, b, c, ca, cb, cc);
	tri(a, c, d, ca, cc, cd);
}
void Canvas::rect(float x, float y, float w, float h, sf::Color col) {
	quad(V2(x, y), V2(x + w, y), V2(x + w, y + h), V2(x, y + h), col);
}
void Canvas::rectV(float x, float y, float w, float h, sf::Color top, sf::Color bottom) {
	quad(V2(x, y), V2(x + w, y), V2(x + w, y + h), V2(x, y + h), top, top, bottom, bottom);
}
void Canvas::rectH(float x, float y, float w, float h, sf::Color left, sf::Color right) {
	quad(V2(x, y), V2(x + w, y), V2(x + w, y + h), V2(x, y + h), left, right, right, left);
}
void Canvas::rotRect(V2 c, V2 h, float ang, sf::Color col) {
	quad(c + rot(V2(-h.x, -h.y), ang), c + rot(V2(h.x, -h.y), ang), c + rot(V2(h.x, h.y), ang),
	     c + rot(V2(-h.x, h.y), ang), col);
}
void Canvas::circle(V2 c, float r, sf::Color col, int segs) {
	if (segs <= 0) segs = autoSegs(r);
	V2 prev = c + V2(r, 0.f);
	for (int i = 1; i <= segs; ++i) {
		float a = 2.f * PI * i / segs;
		V2 p = c + V2(std::cos(a) * r, std::sin(a) * r);
		tri(c, prev, p, col);
		prev = p;
	}
}
void Canvas::circleGrad(V2 c, float r, sf::Color inner, sf::Color outer, int segs) {
	if (segs <= 0) segs = autoSegs(r);
	V2 prev = c + V2(r, 0.f);
	for (int i = 1; i <= segs; ++i) {
		float a = 2.f * PI * i / segs;
		V2 p = c + V2(std::cos(a) * r, std::sin(a) * r);
		tri(c, prev, p, inner, outer, outer);
		prev = p;
	}
}
void Canvas::ellipse(V2 c, float rx, float ry, sf::Color col, float ang, int segs) {
	if (segs <= 0) segs = autoSegs(std::max(rx, ry));
	V2 prev = c + rot(V2(rx, 0.f), ang);
	for (int i = 1; i <= segs; ++i) {
		float a = 2.f * PI * i / segs;
		V2 p = c + rot(V2(std::cos(a) * rx, std::sin(a) * ry), ang);
		tri(c, prev, p, col);
		prev = p;
	}
}
void Canvas::ring(V2 c, float r, float w, sf::Color col, int segs) {
	if (segs <= 0) segs = autoSegs(r);
	arc(c, r, w, 0.f, 2.f * PI, col, segs);
}
void Canvas::arc(V2 c, float r, float w, float a0, float a1, sf::Color col, int segs) {
	float ri = r - w * 0.5f, ro = r + w * 0.5f;
	for (int i = 0; i < segs; ++i) {
		float t0 = a0 + (a1 - a0) * i / segs, t1 = a0 + (a1 - a0) * (i + 1) / segs;
		V2 d0(std::cos(t0), std::sin(t0)), d1(std::cos(t1), std::sin(t1));
		quad(c + d0 * ri, c + d0 * ro, c + d1 * ro, c + d1 * ri, col);
	}
}
void Canvas::pie(V2 c, float r, float a0, float a1, sf::Color col, int segs) {
	for (int i = 0; i < segs; ++i) {
		float t0 = a0 + (a1 - a0) * i / segs, t1 = a0 + (a1 - a0) * (i + 1) / segs;
		tri(c, c + V2(std::cos(t0), std::sin(t0)) * r, c + V2(std::cos(t1), std::sin(t1)) * r, col);
	}
}
void Canvas::roundRect(float x, float y, float w, float h, float r, sf::Color col) {
	r = std::min(r, std::min(w, h) * 0.5f);
	rect(x + r, y, w - 2 * r, h, col);
	rect(x, y + r, r, h - 2 * r, col);
	rect(x + w - r, y + r, r, h - 2 * r, col);
	pie(V2(x + r, y + r), r, PI, 1.5f * PI, col);
	pie(V2(x + w - r, y + r), r, 1.5f * PI, 2.f * PI, col);
	pie(V2(x + w - r, y + h - r), r, 0.f, 0.5f * PI, col);
	pie(V2(x + r, y + h - r), r, 0.5f * PI, PI, col);
}
void Canvas::roundRectRot(V2 c, V2 h, float r, float ang, sf::Color col) {
	r = std::min(r, std::min(h.x, h.y));
	std::vector<V2> pts;
	const V2 corners[4] = {V2(h.x - r, h.y - r), V2(-h.x + r, h.y - r), V2(-h.x + r, -h.y + r), V2(h.x - r, -h.y + r)};
	for (int k = 0; k < 4; ++k) {
		for (int i = 0; i <= 4; ++i) {
			float a = PI * 0.5f * k + PI * 0.5f * i / 4.f;
			pts.push_back(c + rot(corners[k] + V2(std::cos(a) * r, std::sin(a) * r), ang));
		}
	}
	polygon(pts, col);
}
void Canvas::line(V2 a, V2 b, float w, sf::Color col) { line(a, b, w, col, col); }
void Canvas::line(V2 a, V2 b, float w, sf::Color ca, sf::Color cb) {
	V2 n = perp(norm(b - a)) * (w * 0.5f);
	quad(a + n, b + n, b - n, a - n, ca, cb, cb, ca);
}
void Canvas::taper(V2 a, V2 b, float wa, float wb, sf::Color col) {
	V2 n = perp(norm(b - a));
	quad(a + n * (wa * 0.5f), b + n * (wb * 0.5f), b - n * (wb * 0.5f), a - n * (wa * 0.5f), col);
}
void Canvas::polyline(const std::vector<V2>& pts, float w, sf::Color col, bool roundJoins) {
	if (pts.size() < 2) return;
	for (size_t i = 0; i + 1 < pts.size(); ++i) line(pts[i], pts[i + 1], w, col);
	if (roundJoins && w > 2.5f)
		for (size_t i = 1; i + 1 < pts.size(); ++i) circle(pts[i], w * 0.5f, col, 8);
}
void Canvas::polygon(const std::vector<V2>& pts, sf::Color col) {
	for (size_t i = 1; i + 1 < pts.size(); ++i) tri(pts[0], pts[i], pts[i + 1], col);
}
void Canvas::leaf(V2 base, V2 dir, float length, float width, sf::Color col, sf::Color vein) {
	V2 n = perp(dir);
	const int N = 7;
	std::vector<V2> left, right;
	for (int i = 0; i <= N; ++i) {
		float t = static_cast<float>(i) / N;
		float wv = std::sin(t * PI) * width * (1.f - 0.25f * t);
		V2 p = base + dir * (t * length);
		left.push_back(p + n * wv);
		right.push_back(p - n * wv);
	}
	for (int i = 0; i < N; ++i) {
		quad(left[i], left[i + 1], right[i + 1], right[i], col);
	}
	if (vein.a > 0) line(base, base + dir * (length * 0.85f), std::max(1.f, width * 0.18f), vein);
}
void Canvas::star(V2 c, float r, float inner, int points, float ang, sf::Color col) {
	std::vector<V2> pts;
	for (int i = 0; i < points * 2; ++i) {
		float a = ang + PI * i / points;
		float rr = (i % 2) ? inner : r;
		pts.push_back(c + V2(std::cos(a) * rr, std::sin(a) * rr));
	}
	for (size_t i = 0; i < pts.size(); ++i) tri(c, pts[i], pts[(i + 1) % pts.size()], col);
}

std::vector<V2> catmullRom(const std::vector<V2>& p, int n) {
	std::vector<V2> out;
	if (p.size() < 2) return p;
	for (size_t i = 0; i + 1 < p.size(); ++i) {
		V2 p0 = i > 0 ? p[i - 1] : p[i] * 2.f - p[i + 1];
		V2 p1 = p[i], p2 = p[i + 1];
		V2 p3 = i + 2 < p.size() ? p[i + 2] : p[i + 1] * 2.f - p[i];
		for (int k = 0; k < n; ++k) {
			float t = static_cast<float>(k) / n, t2 = t * t, t3 = t2 * t;
			V2 q = (p1 * 2.f + (p2 - p0) * t + (p0 * 2.f - p1 * 5.f + p2 * 4.f - p3) * t2 +
			        (p1 * 3.f - p0 - p2 * 3.f + p3) * t3) *
			       0.5f;
			out.push_back(q);
		}
	}
	out.push_back(p.back());
	return out;
}

std::vector<V2> quadBezier(V2 a, V2 c, V2 b, int n) {
	std::vector<V2> out;
	for (int i = 0; i <= n; ++i) {
		float t = static_cast<float>(i) / n, u = 1.f - t;
		out.push_back(a * (u * u) + c * (2.f * u * t) + b * (t * t));
	}
	return out;
}

sf::FloatRect atlasRect(Tex t) {
	int i = static_cast<int>(t);
	return sf::FloatRect(i * 64.f + 1.f, 1.f, 62.f, 62.f);
}

void SpriteBatch::add(Tex t, V2 c, V2 h, float ang, sf::Color col) {
	sf::FloatRect r = atlasRect(t);
	V2 corners[4] = {V2(-h.x, -h.y), V2(h.x, -h.y), V2(h.x, h.y), V2(-h.x, h.y)};
	V2 uv[4] = {V2(r.left, r.top), V2(r.left + r.width, r.top), V2(r.left + r.width, r.top + r.height),
	            V2(r.left, r.top + r.height)};
	sf::Vertex v[4];
	for (int i = 0; i < 4; ++i) v[i] = sf::Vertex(c + (ang != 0.f ? rot(corners[i], ang) : corners[i]), col, uv[i]);
	va.append(v[0]);
	va.append(v[1]);
	va.append(v[2]);
	va.append(v[0]);
	va.append(v[2]);
	va.append(v[3]);
}
void SpriteBatch::addStreak(V2 a, V2 b, float w, sf::Color col) {
	sf::FloatRect r = atlasRect(Tex::Solid);
	V2 uv(r.left + r.width * 0.5f, r.top + r.height * 0.5f);
	V2 n = perp(norm(b - a)) * (w * 0.5f);
	sf::Color tail = col;
	tail.a = 0;
	va.append(sf::Vertex(a + n, tail, uv));
	va.append(sf::Vertex(b + n, col, uv));
	va.append(sf::Vertex(b - n, col, uv));
	va.append(sf::Vertex(a + n, tail, uv));
	va.append(sf::Vertex(b - n, col, uv));
	va.append(sf::Vertex(a - n, tail, uv));
}
void SpriteBatch::draw(sf::RenderTarget& target, const sf::Texture& atlas, sf::BlendMode blend) const {
	if (!va.getVertexCount()) return;
	sf::RenderStates st;
	st.texture = &atlas;
	st.blendMode = blend;
	target.draw(va, st);
}
