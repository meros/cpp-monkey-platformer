#include "World.h"

#include "Config.h"
#include "ContactListener.h"

#include <algorithm>
#include <cmath>

using namespace cfg;

namespace {

inline float tileX(int x) { return x * TILE; }
inline float tileY(int y) { return y * TILE; }

b2Fixture* addBox(b2Body* b, float hw, float hh, b2Vec2 c, float density, float friction, uint16 cat,
                  uint16 mask, float angle = 0.f) {
	b2PolygonShape s;
	s.SetAsBox(hw, hh, c, angle);
	b2FixtureDef fd;
	fd.shape = &s;
	fd.density = density;
	fd.friction = friction;
	fd.filter.categoryBits = cat;
	fd.filter.maskBits = mask;
	return b->CreateFixture(&fd);
}

constexpr uint16 MASK_ALL = 0xFFFF & ~CAT_NONE;

struct RayClosest : b2RayCastCallback {
	bool hit = false;
	float fraction = 1.f;
	float ReportFixture(b2Fixture* f, const b2Vec2&, const b2Vec2&, float fr) override {
		if (f->IsSensor()) return -1.f;
		uint16 cat = f->GetFilterData().categoryBits;
		if (!(cat & (CAT_TERRAIN | CAT_OBJECT))) return -1.f;
		hit = true;
		fraction = fr;
		return fr;
	}
};

float smoothstep(float t) {
	t = std::clamp(t, 0.f, 1.f);
	return t * t * (3.f - 2.f * t);
}

} // namespace

World::World(const LevelData& lvl) : level(lvl) {
	// Persistent (non-physics) level contents
	std::vector<std::pair<int, int>> signCells;
	for (int y = 0; y < level.height; ++y) {
		for (int x = 0; x < level.width; ++x) {
			char c = level.at(x, y);
			switch (c) {
			case 'b':
				items.push_back({tileX(x) + 0.25f, tileY(y) + 0.25f, false, false});
				++bananasTotal;
				break;
			case 'G':
				items.push_back({tileX(x) + 0.25f, tileY(y) + 0.25f, true, false});
				break;
			case 'C':
				checkpoints.push_back({x, y, false});
				break;
			case 'S':
				signCells.push_back({x, y});
				break;
			case 'E':
				exitX = x;
				exitY = y;
				break;
			case 'P':
				startX = x;
				startY = y;
				break;
			case '^': {
				int dir = 0;
				if (level.solid(x, y + 1)) dir = 0;
				else if (level.solid(x, y - 1)) dir = 1;
				else if (level.solid(x + 1, y)) dir = 2;
				else if (level.solid(x - 1, y)) dir = 3;
				thorns.push_back({x, y, dir});
				break;
			}
			default:
				break;
			}
		}
	}
	for (size_t i = 0; i < signCells.size(); ++i)
		signs.push_back({signCells[i].first, signCells[i].second,
		                 i < level.signs.size() ? level.signs[i] : std::string()});
	build();
}

World::~World() {
	player.body = nullptr;
}

b2Vec2 World::spawnPoint() const {
	int x = startX, y = startY;
	if (activeCheckpoint >= 0) {
		x = checkpoints[activeCheckpoint].tx;
		y = checkpoints[activeCheckpoint].ty;
	}
	return b2Vec2(tileX(x) + 0.25f, tileY(y + 1));
}

void World::build() {
	ropes.clear();
	bridges.clear();
	crates.clear();
	boulders.clear();
	floatLogs.clear();
	fallLogs.clear();
	beetles.clear();
	movers.clear();
	seesaws.clear();
	lifts.clear();
	crumbles.clear();
	mushrooms.clear();
	b2.reset();
	b2 = std::make_unique<b2World>(b2Vec2(0.f, GRAVITY));
	contacts = std::make_unique<ContactListener>(*this);
	b2->SetContactListener(contacts.get());

	int facing = player.facing;
	player = Player();
	player.facing = facing;

	buildTerrain();
	buildObjects();
	player.create(*b2, spawnPoint());
}

void World::buildTerrain() {
	b2BodyDef bd;
	ground = b2->CreateBody(&bd);
	setEntity(ground, &terrainEntity);
	// Joint anchor: a fixture-less static body, so jointed objects still collide with terrain.
	anchor = b2->CreateBody(&bd);
	setEntity(anchor, &terrainEntity);

	const int W = level.width, H = level.height;
	std::vector<char> used(W * H, 0);
	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			if (level.at(x, y) != '#' || used[y * W + x]) continue;
			int w = 0;
			while (x + w < W && level.at(x + w, y) == '#' && !used[y * W + x + w]) ++w;
			int h = 1;
			while (y + h < H) {
				bool ok = true;
				for (int i = 0; i < w && ok; ++i)
					ok = level.at(x + i, y + h) == '#' && !used[(y + h) * W + x + i];
				if (!ok) break;
				++h;
			}
			for (int j = 0; j < h; ++j)
				for (int i = 0; i < w; ++i) used[(y + j) * W + x + i] = 1;
			addBox(ground, w * TILE * 0.5f, h * TILE * 0.5f,
			       b2Vec2(tileX(x) + w * TILE * 0.5f, tileY(y) + h * TILE * 0.5f), 0.f, 0.6f,
			       CAT_TERRAIN, MASK_ALL);
		}
	}
	// invisible side walls
	{
		b2EdgeShape e;
		b2FixtureDef fd;
		fd.shape = &e;
		fd.friction = 0.f;
		fd.filter.categoryBits = CAT_TERRAIN;
		fd.filter.maskBits = MASK_ALL;
		e.SetTwoSided(b2Vec2(0.f, -60.f), b2Vec2(0.f, level.heightM() + 20.f));
		ground->CreateFixture(&fd);
		e.SetTwoSided(b2Vec2(level.widthM(), -60.f), b2Vec2(level.widthM(), level.heightM() + 20.f));
		ground->CreateFixture(&fd);
	}
	// river beds: water that reaches the bottom row gets an invisible floor (DESIGN 3.7: water is
	// never lethal, diving cannot drop Pip out of the map)
	for (int x = 0; x < W; ++x) {
		if (!level.waterAt(x, H - 1)) continue;
		int x1 = x;
		while (x1 + 1 < W && level.waterAt(x1 + 1, H - 1)) ++x1;
		b2EdgeShape e;
		e.SetTwoSided(b2Vec2(tileX(x), level.heightM()), b2Vec2(tileX(x1 + 1), level.heightM()));
		b2FixtureDef fd;
		fd.shape = &e;
		fd.friction = 0.6f;
		fd.filter.categoryBits = CAT_TERRAIN;
		fd.filter.maskBits = MASK_ALL;
		ground->CreateFixture(&fd);
		x = x1;
	}
	// slopes
	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			char c = level.at(x, y);
			if (c != '/' && c != '\\') continue;
			b2Vec2 v[3];
			float l = tileX(x), r = tileX(x + 1), t = tileY(y), b = tileY(y + 1);
			if (c == '/') {
				v[0].Set(l, b);
				v[1].Set(r, b);
				v[2].Set(r, t);
			} else {
				v[0].Set(l, b);
				v[1].Set(r, b);
				v[2].Set(l, t);
			}
			b2PolygonShape s;
			s.Set(v, 3);
			b2FixtureDef fd;
			fd.shape = &s;
			fd.friction = 0.6f;
			fd.filter.categoryBits = CAT_TERRAIN;
			fd.filter.maskBits = MASK_ALL;
			ground->CreateFixture(&fd);
		}
	}
	// one-way branches
	{
		b2Body* ow = b2->CreateBody(&bd);
		setEntity(ow, &oneWayEntity);
		for (int y = 0; y < H; ++y) {
			for (int x = 0; x < W; ++x) {
				if (level.at(x, y) != '=') continue;
				int w = 0;
				while (level.at(x + w, y) == '=') ++w;
				addBox(ow, w * TILE * 0.5f, 0.06f, b2Vec2(tileX(x) + w * TILE * 0.5f, tileY(y) + 0.06f), 0.f,
				       0.6f, CAT_TERRAIN, MASK_ALL);
				x += w - 1;
			}
		}
	}
	// crumbling ledges & mushrooms (own bodies)
	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			char c = level.at(x, y);
			if (c == '%') {
				auto cr = std::make_unique<Crumble>();
				cr->tx = x;
				cr->ty = y;
				b2BodyDef cbd;
				cbd.position.Set(tileX(x) + 0.25f, tileY(y) + 0.25f);
				cr->body = b2->CreateBody(&cbd);
				setEntity(cr->body, cr.get());
				addBox(cr->body, 0.25f, 0.25f, b2Vec2(0, 0), 1.f, 0.6f, CAT_TERRAIN, MASK_ALL);
				crumbles.push_back(std::move(cr));
			} else if (c == 'M') {
				auto m = std::make_unique<Mushroom>();
				m->tx = x;
				m->ty = y;
				b2BodyDef mbd;
				mbd.position.Set(tileX(x) + 0.25f, tileY(y));
				m->body = b2->CreateBody(&mbd);
				setEntity(m->body, m.get());
				addBox(m->body, 0.25f, 0.125f, b2Vec2(0.f, 0.125f), 0.f, 0.6f, CAT_TERRAIN, MASK_ALL);
				addBox(m->body, 0.10f, 0.125f, b2Vec2(0.f, 0.375f), 0.f, 0.6f, CAT_TERRAIN, MASK_ALL);
				mushrooms.push_back(std::move(m));
			}
		}
	}
}

void World::buildObjects() {
	const int W = level.width, H = level.height;
	const uint16 objMask = CAT_TERRAIN | CAT_PLAYER | CAT_OBJECT | CAT_BEETLE;

	for (int y = 0; y < H; ++y) {
		for (int x = 0; x < W; ++x) {
			char c = level.at(x, y);
			if (c == 'R') {
				// ---- vine ----
				int len = 1;
				while (level.at(x, y + len) == '|') ++len;
				auto r = std::make_unique<Rope>();
				r->lengthTiles = len;
				r->anchor.Set(tileX(x) + 0.25f, tileY(y));
				int n = 2 * len;
				b2Body* prev = anchor;
				for (int i = 0; i < n; ++i) {
					b2BodyDef sbd;
					sbd.type = b2_dynamicBody;
					sbd.position.Set(r->anchor.x, r->anchor.y + ROPE_SEG_LEN * (i + 0.5f));
					sbd.linearDamping = 0.6f;
					sbd.angularDamping = 0.5f;
					b2Body* sb = b2->CreateBody(&sbd);
					setEntity(sb, r.get());
					b2PolygonShape s;
					s.SetAsBox(ROPE_SEG_W * 0.5f, ROPE_SEG_LEN * 0.5f);
					b2FixtureDef fd;
					fd.shape = &s;
					fd.density = ROPE_SEG_MASS / (ROPE_SEG_W * ROPE_SEG_LEN);
					fd.isSensor = true;
					fd.filter.categoryBits = CAT_NONE;
					fd.filter.maskBits = 0;
					sb->CreateFixture(&fd);
					b2RevoluteJointDef jd;
					b2Vec2 at(r->anchor.x, r->anchor.y + ROPE_SEG_LEN * i);
					jd.Initialize(prev, sb, at);
					if (i > 0) {
						jd.enableLimit = true;
						jd.lowerAngle = -0.6f;
						jd.upperAngle = 0.6f;
					}
					b2->CreateJoint(&jd);
					r->segs.push_back(sb);
					prev = sb;
				}
				b2DistanceJointDef dd;
				dd.bodyA = anchor;
				dd.bodyB = r->segs.back();
				dd.localAnchorA = r->anchor;
				dd.localAnchorB.Set(0.f, ROPE_SEG_LEN * 0.5f);
				dd.minLength = 0.f;
				dd.maxLength = n * ROPE_SEG_LEN * 1.02f;
				dd.length = dd.maxLength;
				b2->CreateJoint(&dd);
				ropes.push_back(std::move(r));
			} else if (c == '-' && level.at(x - 1, y) != '-') {
				// ---- rope bridge ----
				int n = 0;
				while (level.at(x + n, y) == '-') ++n;
				auto br = std::make_unique<Bridge>();
				float yTop = tileY(y) + 0.05f;
				br->leftAnchor.Set(tileX(x), yTop);
				br->rightAnchor.Set(tileX(x + n), yTop);
				const float link = TILE * 1.02f;
				b2Body* prev = anchor;
				b2Vec2 prevAnchor = br->leftAnchor;
				for (int i = 0; i < n; ++i) {
					b2BodyDef pbd;
					pbd.type = b2_dynamicBody;
					// pre-sag the planks along a parabola so the bridge starts near rest
					float u = (i + 0.5f) / n;
					float sag = 4.f * u * (1.f - u) * 0.1f * std::sqrt(static_cast<float>(n));
					pbd.position.Set(tileX(x) + TILE * (i + 0.5f), yTop + sag);
					pbd.linearDamping = 1.0f;
					pbd.angularDamping = 2.0f;
					b2Body* pb = b2->CreateBody(&pbd);
					setEntity(pb, br.get());
					addBox(pb, 0.23f, 0.05f, b2Vec2(0, 0), 0.15f / (0.46f * 0.10f), 0.8f, CAT_OBJECT, objMask);
					b2RevoluteJointDef jd;
					jd.bodyA = prev;
					jd.bodyB = pb;
					jd.localAnchorA = prev == anchor ? prevAnchor : b2Vec2(link * 0.5f, 0.f);
					jd.localAnchorB.Set(-link * 0.5f, 0.f);
					b2->CreateJoint(&jd);
					br->planks.push_back(pb);
					prev = pb;
				}
				b2RevoluteJointDef jd;
				jd.bodyA = prev;
				jd.bodyB = anchor;
				jd.localAnchorA.Set(link * 0.5f, 0.f);
				jd.localAnchorB = br->rightAnchor;
				b2->CreateJoint(&jd);
				// Sag cap (3.5): rope limits from both anchors to every plank stop the light
				// chain from stretching under Pip's weight.
				for (int i = 0; i < n; ++i) {
					for (int side = 0; side < 2; ++side) {
						b2DistanceJointDef dj;
						dj.bodyA = anchor;
						dj.bodyB = br->planks[i];
						dj.localAnchorA = side == 0 ? br->leftAnchor : br->rightAnchor;
						dj.localAnchorB.SetZero();
						float along = side == 0 ? (i + 0.5f) * link : (n - i - 0.5f) * link;
						dj.minLength = 0.f;
						dj.maxLength = along;
						dj.length = along;
						b2->CreateJoint(&dj);
					}
				}
				bridges.push_back(std::move(br));
			} else if (c == 'X') {
				auto cr = std::make_unique<Crate>();
				b2BodyDef cbd;
				cbd.type = b2_dynamicBody;
				cbd.position.Set(tileX(x) + 0.25f, tileY(y) + 0.26f);
				cbd.angularDamping = 5.f;
				cr->body = b2->CreateBody(&cbd);
				setEntity(cr->body, cr.get());
				addBox(cr->body, 0.24f, 0.24f, b2Vec2(0, 0), 1.5f / (0.48f * 0.48f), 0.5f, CAT_OBJECT, objMask);
				crates.push_back(std::move(cr));
			} else if (c == 'O') {
				auto bo = std::make_unique<Boulder>();
				b2BodyDef obd;
				obd.type = b2_dynamicBody;
				obd.position.Set(tileX(x) + 0.25f, tileY(y) + 0.25f);
				obd.angularDamping = 0.2f;
				bo->body = b2->CreateBody(&obd);
				setEntity(bo->body, bo.get());
				b2CircleShape cs;
				cs.m_radius = 0.475f;
				b2FixtureDef fd;
				fd.shape = &cs;
				fd.density = 4.f / (b2_pi * 0.475f * 0.475f);
				fd.friction = 0.6f;
				fd.restitution = 0.1f;
				fd.filter.categoryBits = CAT_OBJECT;
				fd.filter.maskBits = objMask;
				bo->body->CreateFixture(&fd);
				boulders.push_back(std::move(bo));
			} else if ((c == 'L' || c == 'F') && level.at(x - 1, y) != c) {
				int n = 0;
				while (level.at(x + n, y) == c) ++n;
				float hw = n * TILE * 0.5f;
				b2BodyDef lbd;
				lbd.position.Set(tileX(x) + hw, tileY(y) + 0.25f);
				if (c == 'L') {
					auto lg = std::make_unique<FloatLog>();
					lg->tiles = n;
					lbd.type = b2_dynamicBody;
					lbd.angularDamping = 3.f;
					lg->body = b2->CreateBody(&lbd);
					setEntity(lg->body, lg.get());
					addBox(lg->body, hw, 0.25f, b2Vec2(0, 0), 0.8f / (2.f * hw * 0.5f), 0.9f, CAT_OBJECT, objMask);
					floatLogs.push_back(std::move(lg));
				} else {
					auto lg = std::make_unique<FallLog>();
					lg->tiles = n;
					lg->x0 = x;
					lg->y0 = y;
					lbd.type = b2_staticBody;
					lbd.fixedRotation = true;
					lg->body = b2->CreateBody(&lbd);
					setEntity(lg->body, lg.get());
					addBox(lg->body, hw, 0.25f, b2Vec2(0, 0), 4.f / (2.f * hw * 0.5f), 0.8f, CAT_OBJECT, objMask);
					fallLogs.push_back(std::move(lg));
				}
			} else if (c == 'e') {
				auto be = std::make_unique<Beetle>();
				b2BodyDef ebd;
				ebd.type = b2_dynamicBody;
				ebd.fixedRotation = true;
				ebd.position.Set(tileX(x) + 0.25f, tileY(y + 1) - 0.125f - 0.005f);
				be->body = b2->CreateBody(&ebd);
				setEntity(be->body, be.get());
				addBox(be->body, 0.2f, 0.125f, b2Vec2(0, 0), 0.3f / (0.4f * 0.25f), 0.f, CAT_BEETLE,
				       CAT_TERRAIN | CAT_OBJECT);
				be->dir = (x % 2) ? 1 : -1;
				be->walkPhase = x * 0.37f;
				beetles.push_back(std::move(be));
			}
		}
	}

	// ---- digit objects ----
	for (int d = 0; d < 10; ++d) {
		const ObjDef& o = level.objs[d];
		if (!o.defined || o.x < 0) continue;
		int w = static_cast<int>(o.get("w", 3));
		if (o.type == "mover") {
			auto m = std::make_unique<Mover>();
			m->w = w;
			m->start.Set(tileX(o.x) + w * TILE * 0.5f, tileY(o.y) + 0.15f);
			m->end = m->start + b2Vec2(o.get("dx", 0) * TILE, o.get("dy", 0) * TILE);
			m->tripTime = std::max(0.2f, o.get("t", 3));
			m->lilyPad = level.biome == Biome::Swamp || level.biome == Biome::River || level.biome == Biome::Storm;
			m->flower = (d % 3 == 1);
			// Even-numbered movers run half a cycle out of phase, so neighbouring pads meet at
			// their near ends (level 8: pad 1's far end is 4 tiles from pad 2's near end).
			if (d % 2 == 0) m->t = m->tripTime + 0.4f;
			b2BodyDef mbd;
			mbd.type = b2_kinematicBody;
			mbd.position = m->t > 0.f ? m->end : m->start;
			m->body = b2->CreateBody(&mbd);
			setEntity(m->body, m.get());
			addBox(m->body, w * TILE * 0.5f, 0.15f, b2Vec2(0, 0), 1.f, 1.0f, CAT_OBJECT, objMask);
			movers.push_back(std::move(m));
		} else if (o.type == "seesaw") {
			auto s = std::make_unique<Seesaw>();
			s->w = w;
			s->limitDeg = o.get("limit", 25);
			s->pivot.Set(tileX(o.x) + 0.25f, tileY(o.y) + 0.25f);
			b2BodyDef sbd;
			sbd.type = b2_dynamicBody;
			sbd.position = s->pivot;
			sbd.angularDamping = 1.f;
			s->body = b2->CreateBody(&sbd);
			setEntity(s->body, s.get());
			// The steep puzzle see-saw (limit > 30 deg) gets a grippy plank and low end stops so
			// its crate stays aboard (docs/DESIGN.md 3.12, implementation notes).
			bool puzzle = s->limitDeg > 30.f;
			addBox(s->body, w * TILE * 0.5f, 0.075f, b2Vec2(0, 0), 1.5f / (w * TILE * 0.15f), puzzle ? 1.6f : 0.8f,
			       CAT_OBJECT, objMask);
			if (puzzle) {
				s->lips = true;
				for (int side = -1; side <= 1; side += 2)
					addBox(s->body, 0.04f, 0.06f, b2Vec2(side * (w * TILE * 0.5f - 0.04f), -0.13f), 0.f, 0.8f,
					       CAT_OBJECT, objMask);
			}
			b2RevoluteJointDef jd;
			jd.Initialize(anchor, s->body, s->pivot);
			jd.enableLimit = true;
			jd.lowerAngle = -s->limitDeg * b2_pi / 180.f;
			jd.upperAngle = s->limitDeg * b2_pi / 180.f;
			b2->CreateJoint(&jd);
			seesaws.push_back(std::move(s));
		}
	}
	// A crate spawned above a see-saw starts resting on it, with the plank already tipped to its
	// limit on that side (DESIGN 4.3 level 6: "a crate starts on the right end ... holding it
	// down as a ramp"). Dropping it on a level plank would slam the plank and fling the crate.
	for (auto& ss : seesaws) {
		float hw = ss->w * TILE * 0.5f;
		for (auto& cr : crates) {
			b2Vec2 cp = cr->body->GetPosition();
			float dx = cp.x - ss->pivot.x;
			if (std::fabs(dx) > hw - 0.1f || cp.y > ss->pivot.y || cp.y < ss->pivot.y - 1.6f) continue;
			float ang = (dx > 0.f ? 1.f : -1.f) * ss->limitDeg * b2_pi / 180.f;
			ss->body->SetTransform(ss->pivot, ang);
			b2Rot r(ang);
			b2Vec2 local(dx, -(0.075f + 0.24f + 0.004f));
			cr->body->SetTransform(ss->pivot + b2Mul(r, local), ang);
		}
	}
	// pulleys: create every platform, then one joint per pair
	Lift* byDigit[10] = {nullptr};
	for (int d = 0; d < 10; ++d) {
		const ObjDef& o = level.objs[d];
		if (!o.defined || o.x < 0 || o.type != "pulley") continue;
		int w = static_cast<int>(o.get("w", 3));
		auto l = std::make_unique<Lift>();
		l->w = w;
		l->wheel.Set((o.x + w * 0.5f) * TILE, tileY(static_cast<int>(o.get("top", 0))) + 0.25f);
		b2BodyDef lbd;
		lbd.type = b2_dynamicBody;
		lbd.fixedRotation = true;
		lbd.linearDamping = o.get("damping", 1.f);
		l->maxSpeed = 2.4f / std::max(0.25f, o.get("damping", 1.f));
		lbd.position.Set((o.x + w * 0.5f) * TILE, tileY(o.y) + 0.2f);
		l->body = b2->CreateBody(&lbd);
		setEntity(l->body, l.get());
		// 2 cm narrower than its shaft so it never binds on the walls
		addBox(l->body, w * TILE * 0.5f - 0.02f, 0.2f, b2Vec2(0, 0), o.get("mass", 1.f) / ((w * TILE - 0.04f) * 0.4f), 1.0f,
		       CAT_OBJECT, objMask);
		b2PrismaticJointDef pj;
		pj.Initialize(anchor, l->body, l->body->GetPosition(), b2Vec2(0.f, 1.f));
		b2->CreateJoint(&pj);
		byDigit[d] = l.get();
		lifts.push_back(std::move(l));
	}
	for (int d = 0; d < 10; ++d) {
		if (!byDigit[d]) continue;
		int p = static_cast<int>(level.objs[d].get("partner", -1));
		if (p < 0 || p > 9 || !byDigit[p]) continue;
		byDigit[d]->partner = byDigit[p];
		if (d > p) continue;
		float ratio = level.objs[d].get("ratio", level.objs[p].get("ratio", 1.f));
		byDigit[d]->primary = true;
		Lift* a = byDigit[d];
		Lift* b = byDigit[p];
		// speed(a) = ratio * speed(b): cap the pair by the stricter of the two platforms
		float capB = std::min(b->maxSpeed, a->maxSpeed / ratio);
		a->maxSpeed = capB * ratio;
		b->maxSpeed = capB;
		b2PulleyJointDef pd;
		b2Vec2 aa = a->body->GetPosition() - b2Vec2(0.f, 0.2f);
		b2Vec2 ba = b->body->GetPosition() - b2Vec2(0.f, 0.2f);
		pd.Initialize(a->body, b->body, a->wheel, b->wheel, aa, ba, ratio);
		pd.collideConnected = true;
		b2->CreateJoint(&pd);
	}
}

// ---------------------------------------------------------------------------
// Queries

uint8_t World::waterAtM(float x, float y) const {
	return level.waterAt(static_cast<int>(std::floor(x / TILE)), static_cast<int>(std::floor(y / TILE)));
}

uint8_t World::windAtM(float x, float y) const {
	return level.windAt(static_cast<int>(std::floor(x / TILE)), static_cast<int>(std::floor(y / TILE)));
}

float World::waterSurfaceAt(float x, float y) const {
	int tx = static_cast<int>(std::floor(x / TILE));
	int ty = static_cast<int>(std::floor(y / TILE));
	while (ty > 0 && level.waterAt(tx, ty - 1)) --ty;
	return tileY(ty);
}

bool World::solidTileAtM(float x, float y) const {
	return level.solid(static_cast<int>(std::floor(x / TILE)), static_cast<int>(std::floor(y / TILE)));
}

int World::windDirAt(float x, float y) const {
	if (!windActive()) return 0;
	uint8_t w = windAtM(x, y);
	return w == WI_LEFT ? -1 : w == WI_RIGHT ? 1 : 0;
}

void World::placePlayer(float x, float y) {
	if (player.state == PState::Rope) player.releaseRope(*this, false, true);
	player.body->SetEnabled(true);
	player.body->SetTransform(b2Vec2(x, y), 0.f);
	player.body->SetLinearVelocity(b2Vec2(0, 0));
	player.state = PState::Air;
	player.windVel = 0.f;
}

// ---------------------------------------------------------------------------
// Simulation

void World::killPlayer() { player.kill(*this); }

void World::respawn() {
	build();
	player.state = PState::Air;
	emit(Ev::Respawn, player.pos().x, player.pos().y);
}

void World::updateGust(float dt) {
	(void)dt;
	if (!level.gust) {
		gustOn = true;
		windEnvelope = 1.f;
		return;
	}
	float cycle = level.gustOn + level.gustOff;
	float t = std::fmod(time, cycle);
	bool on = t < level.gustOn;
	if (on && !gustOn) emit(Ev::GustStart, 0, 0);
	if (!on && gustOn) emit(Ev::GustEnd, 0, 0);
	gustOn = on;
	bool warn = !on && t >= cycle - 0.6f;
	if (warn && !gustWarned) emit(Ev::GustWarn, 0, 0);
	gustWarned = warn;
	if (on) windEnvelope = std::min(1.f, 0.5f + t / 0.3f);
	else if (warn) windEnvelope = 0.5f * (t - (cycle - 0.6f)) / 0.6f;
	else windEnvelope = std::max(0.f, 1.f - (t - level.gustOn) / 0.6f);
}

void World::applyWaterForces(b2Body* b, float floatFrac) {
	float submergedW = 0.f, total = 0.f, cxSum = 0.f, cySum = 0.f, flow = 0.f;
	for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext()) {
		const b2Shape* s = f->GetShape();
		std::vector<b2Vec2> pts;
		if (s->GetType() == b2Shape::e_polygon) {
			const b2PolygonShape* ps = static_cast<const b2PolygonShape*>(s);
			b2Vec2 lo = ps->m_vertices[0], hi = lo;
			for (int i = 1; i < ps->m_count; ++i) {
				lo = b2Min(lo, ps->m_vertices[i]);
				hi = b2Max(hi, ps->m_vertices[i]);
			}
			int nx = std::max(2, static_cast<int>((hi.x - lo.x) / 0.1f));
			for (int i = 0; i < nx; ++i)
				for (int j = 0; j < 5; ++j)
					pts.emplace_back(lo.x + (hi.x - lo.x) * (i + 0.5f) / nx, lo.y + (hi.y - lo.y) * (j + 0.5f) / 5.f);
		} else if (s->GetType() == b2Shape::e_circle) {
			const b2CircleShape* cs = static_cast<const b2CircleShape*>(s);
			for (int i = 0; i < 7; ++i)
				for (int j = 0; j < 7; ++j) {
					b2Vec2 q(cs->m_radius * ((i + 0.5f) / 3.5f - 1.f), cs->m_radius * ((j + 0.5f) / 3.5f - 1.f));
					if (q.LengthSquared() <= cs->m_radius * cs->m_radius) pts.push_back(cs->m_p + q);
				}
		}
		for (const b2Vec2& lp : pts) {
			b2Vec2 wp = b->GetWorldPoint(lp);
			total += 1.f;
			uint8_t w = waterAtM(wp.x, wp.y);
			if (w) {
				submergedW += 1.f;
				cxSum += wp.x;
				cySum += wp.y;
				if (w == W_LEFT) flow -= 1.f;
				if (w == W_RIGHT) flow += 1.f;
			}
		}
	}
	if (total <= 0.f) return;
	float frac = submergedW / total;
	Entity* e = entityOf(b);
	float baseLin = 0.f, baseAng = 0.f;
	if (e && e->kind == Kind::FloatLog) baseAng = 3.f;
	if (e && e->kind == Kind::Crate) baseAng = 5.f;
	if (e && e->kind == Kind::Boulder) baseAng = 0.2f;
	if (frac <= 0.f) {
		b->SetLinearDamping(baseLin);
		b->SetAngularDamping(baseAng);
		return;
	}
	float m = b->GetMass();
	b2Vec2 c(cxSum / submergedW, cySum / submergedW);
	b->ApplyForce(b2Vec2(0.f, -m * GRAVITY * frac / floatFrac), c, true);
	// area of the body approximated from mass-independent sampling: use AABB area of fixtures
	float area = 0.f;
	for (b2Fixture* f = b->GetFixtureList(); f; f = f->GetNext()) {
		b2MassData md;
		f->GetMassData(&md);
		if (f->GetDensity() > 0.f) area += md.mass / f->GetDensity();
	}
	float flowArea = area * (flow / total);
	b->ApplyForceToCenter(b2Vec2(1.2f * flowArea / (TILE * TILE), 0.f), true);
	b->SetLinearDamping(2.f);
	b->SetAngularDamping(3.f);
}

void World::updateObjects(float dt) {
	// crumbles
	for (auto& c : crumbles) {
		c->timer += dt;
		switch (c->state) {
		case Crumble::Shaking:
			if (c->timer >= 0.35f) {
				c->state = Crumble::Falling;
				c->fallTime = 0.f;
				c->body->SetType(b2_dynamicBody);
				c->body->SetAwake(true);
				emit(Ev::CrumbleFall, c->body->GetPosition().x, c->body->GetPosition().y);
			}
			break;
		case Crumble::Falling:
			c->fallTime += dt;
			if (c->fallTime >= 0.15f) {
				b2Filter f;
				f.categoryBits = CAT_OBJECT;
				f.maskBits = CAT_TERRAIN | CAT_OBJECT;
				c->body->GetFixtureList()->SetFilterData(f);
			}
			c->alpha = std::max(0.f, 1.f - c->fallTime / 0.6f);
			if (c->fallTime >= 0.6f) {
				c->state = Crumble::Gone;
				c->body->SetEnabled(false);
			}
			break;
		case Crumble::Gone: {
			c->fallTime += dt;
			if (c->fallTime >= 3.f) {
				// only return if nothing overlaps the cell
				float cx = c->tx * TILE + 0.25f, cy = c->ty * TILE + 0.25f;
				b2Vec2 pp = player.pos();
				bool blocked = std::fabs(pp.x - cx) < 0.45f && std::fabs(pp.y - cy) < 0.55f;
				if (!blocked) {
					c->body->SetEnabled(true);
					c->body->SetType(b2_staticBody);
					c->body->SetTransform(b2Vec2(cx, cy), 0.f);
					c->body->SetLinearVelocity(b2Vec2(0, 0));
					c->body->SetAngularVelocity(0.f);
					b2Filter f;
					f.categoryBits = CAT_TERRAIN;
					f.maskBits = 0xFFFF & ~CAT_NONE;
					c->body->GetFixtureList()->SetFilterData(f);
					c->state = Crumble::Returning;
					c->timer = 0.f;
					c->alpha = 0.f;
					emit(Ev::CrumbleReturn, cx, cy);
				}
			}
			break;
		}
		case Crumble::Returning:
			c->alpha = std::min(1.f, c->timer / 0.3f);
			if (c->alpha >= 1.f) c->state = Crumble::Solid;
			break;
		default:
			break;
		}
	}
	for (auto& m : mushrooms) m->squash = std::max(0.f, m->squash - dt / 0.25f);

	// movers
	for (auto& m : movers) {
		m->t += dt;
		const float pause = 0.4f;
		float cycle = 2.f * (m->tripTime + pause);
		auto posAt = [&](float t) {
			t = std::fmod(t, cycle);
			float u;
			if (t < pause) u = 0.f;
			else if (t < pause + m->tripTime) u = smoothstep((t - pause) / m->tripTime);
			else if (t < 2.f * pause + m->tripTime) u = 1.f;
			else u = 1.f - smoothstep((t - 2.f * pause - m->tripTime) / m->tripTime);
			return m->start + u * (m->end - m->start);
		};
		b2Vec2 want = posAt(m->t + dt);
		b2Vec2 v = (1.f / dt) * (want - m->body->GetPosition());
		m->body->SetLinearVelocity(v);
	}

	// beetles
	for (auto& b : beetles) {
		if (!b->body) continue;
		if (b->dead) {
			b->deadTimer += dt;
			if (b->deadTimer > 1.5f && b->body->IsEnabled()) b->body->SetEnabled(false);
			continue;
		}
		b->walkPhase += dt;
		b2Vec2 p = b->body->GetPosition();
		RayClosest ahead;
		b2->RayCast(&ahead, p, p + b2Vec2(b->dir * 0.32f, 0.f));
		RayClosest down;
		b2Vec2 d0 = p + b2Vec2(b->dir * 0.3f, 0.f);
		b2->RayCast(&down, d0, d0 + b2Vec2(0.f, 0.4f));
		bool atWall = ahead.hit;
		bool atLedge = !down.hit;
		b2Vec2 v = b->body->GetLinearVelocity();
		bool onGround = std::fabs(v.y) < 0.5f;
		if ((atWall || atLedge) && onGround) b->dir = -b->dir;
		v.x = onGround ? b->dir * 1.5f : 0.f;
		b->body->SetLinearVelocity(v);
	}

	// falling logs
	b2Vec2 pp = player.pos();
	for (auto& l : fallLogs) {
		if (l->state == FallLog::Hanging && player.alive()) {
			float lx0 = tileX(l->x0), lx1 = tileX(l->x0 + l->tiles);
			float ly = tileY(l->y0 + 1);
			bool under = pp.x + PLAYER_W * 0.5f > lx0 && pp.x - PLAYER_W * 0.5f < lx1 &&
			             pp.y + PLAYER_H * 0.5f > ly && pp.y - PLAYER_H * 0.5f < ly + 6 * TILE;
			if (under) {
				l->state = FallLog::Triggered;
				l->timer = 0.f;
				emit(Ev::LogCreak, l->body->GetPosition().x, l->body->GetPosition().y);
			}
		}
		if (l->state == FallLog::Triggered) {
			l->timer += dt;
			l->wobble = std::sin(l->timer * 40.f) * 0.03f;
			if (l->timer >= 0.3f) {
				l->state = FallLog::Falling;
				l->wobble = 0.f;
				l->body->SetType(b2_dynamicBody);
				l->body->SetFixedRotation(true);
				l->body->SetAwake(true);
				l->timer = 0.f;
			}
		} else if (l->state == FallLog::Falling) {
			l->timer += dt;
			b2Vec2 v = l->body->GetLinearVelocity();
			l->lastVy = v.y;
			if (l->timer > 0.1f && std::fabs(v.y) < 0.2f) {
				l->state = FallLog::Resting;
				emit(Ev::LogThud, l->body->GetPosition().x, l->body->GetPosition().y + 0.25f);
			}
		}
	}

	// water, wind and current forces on props
	auto props = [&](b2Body* body, float floatFrac) {
		if (!body || !body->IsEnabled() || body->GetType() != b2_dynamicBody) return;
		applyWaterForces(body, floatFrac);
		b2Vec2 p = body->GetPosition();
		if (windActive()) {
			uint8_t w = windAtM(p.x, p.y);
			float m = body->GetMass();
			bool touching = false;
			for (b2ContactEdge* ce = body->GetContactList(); ce && !touching; ce = ce->next)
				touching = ce->contact->IsTouching();
			float a = touching ? WIND_GROUND : WIND_AIR;
			if (w == WI_LEFT) body->ApplyForceToCenter(b2Vec2(-0.3f * a * m, 0.f), true);
			if (w == WI_RIGHT) body->ApplyForceToCenter(b2Vec2(0.3f * a * m, 0.f), true);
			if (w == WI_UP && body->GetLinearVelocity().y > UPDRAFT_MAX_RISE)
				body->ApplyForceToCenter(b2Vec2(0.f, -0.3f * UPDRAFT_ACCEL * m), true);
		}
	};
	for (auto& c : crates)
		if (c->alive) props(c->body, 0.6f);
	for (auto& l : floatLogs)
		if (l->alive) props(l->body, 0.35f);
	for (auto& b : boulders)
		if (b->alive) props(b->body, 1.25f);
	for (auto& l : fallLogs)
		if (l->state != FallLog::Hanging && l->state != FallLog::Triggered) props(l->body, 0.5f);
}

void World::postObjects(float dt) {
	(void)dt;
	// Old mill lifts are slow contraptions: cap each platform's speed (its partner follows via
	// the pulley constraint).
	for (auto& l : lifts) {
		b2Vec2 v = l->body->GetLinearVelocity();
		if (std::fabs(v.y) > l->maxSpeed) {
			v.y = std::copysign(l->maxSpeed, v.y);
			l->body->SetLinearVelocity(v);
		}
	}
	const float killY = level.heightM() + 2.f;
	cratePush = 0.f;
	for (auto& c : crates) {
		if (!c->alive) continue;
		b2Vec2 p = c->body->GetPosition();
		if (p.y > killY) {
			b2->DestroyBody(c->body);
			c->body = nullptr;
			c->alive = false;
			continue;
		}
		c->touchingPlayer = false;
		for (b2ContactEdge* ce = c->body->GetContactList(); ce; ce = ce->next)
			if (ce->contact->IsTouching() && ce->other == player.body) c->touchingPlayer = true;
		if (c->touchingPlayer) {
			b2Vec2 v = c->body->GetLinearVelocity();
			if (std::fabs(v.x) > 3.f) {
				v.x = std::copysign(3.f, v.x);
				c->body->SetLinearVelocity(v);
			}
			if (std::fabs(v.x) > 0.3f) cratePush = std::max(cratePush, std::fabs(v.x) / 3.f);
		}
	}
	boulderRoll = 0.f;
	for (auto& b : boulders) {
		if (!b->alive) continue;
		b2Vec2 p = b->body->GetPosition();
		if (p.y > killY || p.x < -2.f || p.x > level.widthM() + 2.f) {
			b2->DestroyBody(b->body);
			b->body = nullptr;
			b->alive = false;
			continue;
		}
		b2Vec2 v = b->body->GetLinearVelocity();
		b2Vec2 dv = v - b->lastVel;
		if (dv.Length() > 3.f) emit(Ev::BoulderImpact, p.x, p.y + 0.4f, dv.Length());
		b->lastVel = v;
		bool touching = false, touchPlayer = false;
		for (b2ContactEdge* ce = b->body->GetContactList(); ce; ce = ce->next) {
			if (!ce->contact->IsTouching()) continue;
			touching = true;
			if (ce->other == player.body) touchPlayer = true;
		}
		if (touching) boulderRoll = std::max(boulderRoll, std::min(1.f, v.Length() / 6.f));
		// Pip pushes a boulder at walking pace only
		if (touchPlayer && player.inputDir != 0 && v.x * player.inputDir > 1.f &&
		    (b->body->GetPosition().x - player.pos().x) * player.inputDir > 0.f) {
			v.x = static_cast<float>(player.inputDir);
			b->body->SetLinearVelocity(v);
			b->lastVel = v;
		}
	}
	for (auto& l : floatLogs) {
		if (!l->alive) continue;
		// Over the falls: once a log's centre is above a bottomless air column it no longer
		// collides with terrain, so it tumbles away instead of wedging in the gap.
		b2Vec2 lp = l->body->GetPosition();
		int cx = static_cast<int>(std::floor(lp.x / TILE)), cy = static_cast<int>(std::floor(lp.y / TILE));
		if (!waterAtM(lp.x, lp.y)) {
			bool bottomless = true;
			for (int y = std::max(0, cy); y < level.height && bottomless; ++y)
				if (level.solid(cx, y) || level.waterAt(cx, y)) bottomless = false;
			if (bottomless) {
				b2Filter f;
				f.categoryBits = CAT_OBJECT;
				f.maskBits = CAT_PLAYER | CAT_OBJECT;
				l->body->GetFixtureList()->SetFilterData(f);
			}
		}
		if (l->body->GetPosition().y > killY) {
			b2->DestroyBody(l->body);
			l->body = nullptr;
			l->alive = false;
		}
	}
	for (auto& l : fallLogs) {
		if (l->body && l->body->GetPosition().y > killY && l->body->IsEnabled()) l->body->SetEnabled(false);
	}
}

void World::checkItems() {
	if (!player.alive() || player.state == PState::Exiting) return;
	b2Vec2 p = player.pos();
	float pl = p.x - PLAYER_W * 0.5f, pr = p.x + PLAYER_W * 0.5f;
	float pt = p.y - PLAYER_H * 0.5f, pb = p.y + PLAYER_H * 0.5f;
	auto overlapCircle = [&](float cx, float cy, float r) {
		float nx = std::clamp(cx, pl, pr), ny = std::clamp(cy, pt, pb);
		return (nx - cx) * (nx - cx) + (ny - cy) * (ny - cy) < r * r;
	};
	for (auto& it : items) {
		if (it.collected || !overlapCircle(it.x, it.y, 0.18f)) continue;
		it.collected = true;
		if (it.fig) {
			fig = true;
			emit(Ev::Fig, it.x, it.y);
		} else {
			++bananas;
			bananaCombo = comboTimer > 0.f ? std::min(7, bananaCombo + 1) : 0;
			comboTimer = 1.f;
			emit(Ev::Banana, it.x, it.y, static_cast<float>(bananaCombo));
		}
	}
	auto overlapRect = [&](float l, float t, float r, float b) { return pr > l && pl < r && pb > t && pt < b; };
	for (size_t i = 0; i < checkpoints.size(); ++i) {
		Checkpoint& c = checkpoints[i];
		if (!overlapRect(tileX(c.tx), tileY(c.ty - 1), tileX(c.tx + 1), tileY(c.ty + 1))) continue;
		if (static_cast<int>(i) != activeCheckpoint) {
			activeCheckpoint = static_cast<int>(i);
			if (!c.lit) emit(Ev::Checkpoint, tileX(c.tx) + 0.25f, tileY(c.ty));
			c.lit = true;
		}
	}
	for (const Thorn& t : thorns) {
		float cx = tileX(t.tx) + 0.25f, cy = tileY(t.ty) + 0.25f;
		if (overlapRect(cx - 0.18f, cy - 0.18f, cx + 0.18f, cy + 0.18f)) {
			player.kill(*this);
			return;
		}
	}
	if (overlapRect(tileX(exitX), tileY(exitY - 1), tileX(exitX + 1), tileY(exitY + 1))) {
		if (player.state == PState::Rope) player.releaseRope(*this, false, true);
		player.state = PState::Exiting;
		player.exitTimer = 0.f;
		player.exitTargetX = tileX(exitX) + 0.25f;
		emit(Ev::ExitEnter, p.x, p.y);
		return;
	}
	if (p.y > level.heightM() + 1.f) player.kill(*this);
}

void World::step(const Input& in) {
	++frame;
	time += DT;
	if (!complete) levelTimer += DT;
	comboTimer = std::max(0.f, comboTimer - DT);
	updateGust(DT);

	if (in.restartPressed && !complete && player.state != PState::Exiting) {
		++deaths;
		respawn();
		return;
	}

	player.preStep(*this, in, DT);
	updateObjects(DT);
	b2->Step(DT, VEL_ITERS, POS_ITERS);
	player.postStep(*this, DT);
	postObjects(DT);
	checkItems();

	if (player.state == PState::Dead && player.deadTimer >= DEATH_RESPAWN) respawn();
	if (player.state == PState::Exiting && player.exitTimer >= 0.4f && !complete) {
		complete = true;
		emit(Ev::LevelComplete, player.pos().x, player.pos().y);
	}
}
