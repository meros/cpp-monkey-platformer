// Leafwind -- batched procedural drawing helpers. Everything visual in the game is built from
// these triangle batches plus a small texture atlas (docs/DESIGN.md section 5).
#pragma once

#include <SFML/Graphics.hpp>

#include <vector>

using V2 = sf::Vector2f;

inline float dot(V2 a, V2 b) { return a.x * b.x + a.y * b.y; }
float len(V2 a);
V2 norm(V2 a);
V2 rot(V2 a, float ang);
V2 perp(V2 a); // (-y, x)

// Untextured triangle batch.
class Canvas {
public:
	Canvas() : va(sf::Triangles) {}
	void clear() { va.clear(); }
	size_t size() const { return va.getVertexCount(); }

	void tri(V2 a, V2 b, V2 c, sf::Color col);
	void tri(V2 a, V2 b, V2 c, sf::Color ca, sf::Color cb, sf::Color cc);
	void quad(V2 a, V2 b, V2 c, V2 d, sf::Color col);
	void quad(V2 a, V2 b, V2 c, V2 d, sf::Color ca, sf::Color cb, sf::Color cc, sf::Color cd);
	void rect(float x, float y, float w, float h, sf::Color col);
	void rectV(float x, float y, float w, float h, sf::Color top, sf::Color bottom);
	void rectH(float x, float y, float w, float h, sf::Color left, sf::Color right);
	void rotRect(V2 c, V2 half, float ang, sf::Color col);
	void circle(V2 c, float r, sf::Color col, int segs = 0);
	void circleGrad(V2 c, float r, sf::Color inner, sf::Color outer, int segs = 0);
	void ellipse(V2 c, float rx, float ry, sf::Color col, float ang = 0.f, int segs = 0);
	void ring(V2 c, float r, float w, sf::Color col, int segs = 0);
	void arc(V2 c, float r, float w, float a0, float a1, sf::Color col, int segs = 12);
	void pie(V2 c, float r, float a0, float a1, sf::Color col, int segs = 8);
	void roundRect(float x, float y, float w, float h, float r, sf::Color col);
	void roundRectRot(V2 c, V2 half, float r, float ang, sf::Color col);
	void line(V2 a, V2 b, float w, sf::Color col);
	void line(V2 a, V2 b, float w, sf::Color ca, sf::Color cb);
	void taper(V2 a, V2 b, float wa, float wb, sf::Color col);
	void polyline(const std::vector<V2>& pts, float w, sf::Color col, bool roundJoins = true);
	void polygon(const std::vector<V2>& pts, sf::Color col); // convex fan
	// A leaf: base point, direction (unit), length, half-width.
	void leaf(V2 base, V2 dir, float length, float width, sf::Color col, sf::Color vein = sf::Color::Transparent);
	void star(V2 c, float r, float inner, int points, float ang, sf::Color col);

	void draw(sf::RenderTarget& t, const sf::RenderStates& st = sf::RenderStates::Default) const {
		if (va.getVertexCount()) t.draw(va, st);
	}
	sf::VertexArray va;
};

// Catmull-Rom sampling through control points.
std::vector<V2> catmullRom(const std::vector<V2>& pts, int samplesPerSeg);
// Quadratic Bezier samples.
std::vector<V2> quadBezier(V2 a, V2 c, V2 b, int n);

// Atlas regions (see Assets)
enum class Tex { Glow, Leaf, Star, Solid, Drop };

// Textured quad batch using the shared atlas.
class SpriteBatch {
public:
	SpriteBatch() : va(sf::Triangles) {}
	void clear() { va.clear(); }
	void add(Tex t, V2 c, V2 half, float ang, sf::Color col);
	void addStreak(V2 a, V2 b, float w, sf::Color col); // solid texel quad along a->b
	void draw(sf::RenderTarget& target, const sf::Texture& atlas, sf::BlendMode blend) const;
	sf::VertexArray va;
};

sf::FloatRect atlasRect(Tex t);

// The golden leaf of the story (concept-art shape), centred at ctr, w x h px.
void goldenLeaf(Canvas& c, V2 ctr, float w, float h, float ang, sf::Color body, sf::Color vein);
