// Leafwind -- physics objects (docs/DESIGN.md section 3). Every Box2D body's user data points
// at an Entity (common base), so contact callbacks can identify what they touch.
#pragma once

#include <box2d/box2d.h>

#include <string>
#include <vector>

class World;

enum class Kind : unsigned char {
	Terrain,
	OneWay,
	Player,
	RopeSeg,
	Plank,
	Crate,
	Boulder,
	FloatLog,
	FallLog,
	Beetle,
	Mover,
	Seesaw,
	Lift,
	Crumble,
	Mushroom,
};

// Collision categories
enum : uint16 {
	CAT_TERRAIN = 0x0001,
	CAT_PLAYER = 0x0002,
	CAT_OBJECT = 0x0004,
	CAT_BEETLE = 0x0008,
	CAT_NONE = 0x0010,
};

struct Entity {
	explicit Entity(Kind k) : kind(k) {}
	virtual ~Entity() = default;
	Kind kind;
	b2Body* body = nullptr;
};

inline Entity* entityOf(b2Body* b) {
	return reinterpret_cast<Entity*>(b->GetUserData().pointer);
}
inline void setEntity(b2Body* b, Entity* e) { b->GetUserData().pointer = reinterpret_cast<uintptr_t>(e); }

// --- Vines -----------------------------------------------------------------
struct Rope : Entity {
	Rope() : Entity(Kind::RopeSeg) {}
	b2Vec2 anchor;               // metres
	std::vector<b2Body*> segs;   // top to bottom
	int lengthTiles = 0;
	b2Vec2 segCenter(int i) const { return segs[i]->GetPosition(); }
	float segAngle(int i) const { return segs[i]->GetAngle(); }
};

// --- Rope bridges ------------------------------------------------------------
struct Bridge : Entity {
	Bridge() : Entity(Kind::Plank) {}
	b2Vec2 leftAnchor, rightAnchor;
	std::vector<b2Body*> planks;
};

// --- Simple dynamic props ----------------------------------------------------
struct Crate : Entity {
	Crate() : Entity(Kind::Crate) {}
	bool touchingPlayer = false;
	bool alive = true;
};

struct Boulder : Entity {
	Boulder() : Entity(Kind::Boulder) {}
	b2Vec2 lastVel{0, 0};
	bool alive = true;
	float rollSpeed = 0.f;
};

struct FloatLog : Entity {
	FloatLog() : Entity(Kind::FloatLog) {}
	int tiles = 3;
	bool alive = true;
};

struct FallLog : Entity {
	FallLog() : Entity(Kind::FallLog) {}
	int tiles = 3;
	int x0 = 0, y0 = 0;   // tile of leftmost cell
	enum State { Hanging, Triggered, Falling, Resting } state = Hanging;
	float timer = 0.f;
	float wobble = 0.f;
	float lastVy = 0.f;   // velocity before the current step
};

struct Beetle : Entity {
	Beetle() : Entity(Kind::Beetle) {}
	int dir = 1;
	bool dead = false;
	float deadTimer = 0.f;
	float walkPhase = 0.f;
};

struct Mover : Entity {
	Mover() : Entity(Kind::Mover) {}
	int w = 3;
	b2Vec2 start, end;   // body centre positions
	float tripTime = 3.f;
	float t = 0.f;       // phase time
	bool lilyPad = true;
	int flower = 0;
};

struct Seesaw : Entity {
	Seesaw() : Entity(Kind::Seesaw) {}
	int w = 7;
	b2Vec2 pivot;
	float limitDeg = 25.f;
	bool lips = false;
};

struct Lift : Entity {
	Lift() : Entity(Kind::Lift) {}
	int w = 3;
	b2Vec2 wheel;        // ground anchor (wheel centre)
	Lift* partner = nullptr;
	bool primary = false;
};

struct Crumble : Entity {
	Crumble() : Entity(Kind::Crumble) {}
	int tx = 0, ty = 0;
	enum State { Solid, Shaking, Falling, Gone, Returning } state = Solid;
	float timer = 0.f;
	float alpha = 1.f;
	float fallTime = 0.f;
};

struct Mushroom : Entity {
	Mushroom() : Entity(Kind::Mushroom) {}
	int tx = 0, ty = 0;
	float squash = 0.f;   // 1 right after a bounce, decays to 0 over 0.25 s
};

// --- Non-physics items -------------------------------------------------------
struct Collectible {
	float x, y;           // centre, metres
	bool fig = false;
	bool collected = false;
};

struct Checkpoint {
	int tx, ty;
	bool lit = false;
};

struct Sign {
	int tx, ty;
	std::string text;
};

struct Thorn {
	int tx, ty;
	int dir; // 0 up, 1 down, 2 left(points left), 3 right
};
