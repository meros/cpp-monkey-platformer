#include "Player.h"

#include "Config.h"
#include "World.h"

#include <algorithm>
#include <cmath>

using namespace cfg;

namespace {

float approach(float v, float target, float delta) {
	if (v < target) return std::min(v + delta, target);
	return std::max(v - delta, target);
}

int sgn(float v) { return (v > 0.f) - (v < 0.f); }

bool isWoodKind(Kind k) {
	return k == Kind::Plank || k == Kind::Lift || k == Kind::Seesaw || k == Kind::Crate ||
	       k == Kind::FloatLog || k == Kind::FallLog;
}

struct QueryAll : b2QueryCallback {
	std::vector<b2Fixture*> hits;
	bool ReportFixture(b2Fixture* f) override {
		hits.push_back(f);
		return true;
	}
};

} // namespace

void Player::create(b2World& w, b2Vec2 bottomCentre) {
	b2BodyDef bd;
	bd.type = b2_dynamicBody;
	bd.position.Set(bottomCentre.x, bottomCentre.y - PLAYER_H * 0.5f);
	bd.fixedRotation = true;
	bd.bullet = true;
	bd.allowSleep = false;
	body = w.CreateBody(&bd);
	setEntity(body, this);

	b2FixtureDef fd;
	fd.friction = 0.f;
	fd.restitution = 0.f;
	fd.density = 1.f;
	fd.filter.categoryBits = CAT_PLAYER;
	fd.filter.maskBits = CAT_TERRAIN | CAT_OBJECT;

	b2PolygonShape box;
	box.SetAsBox(PLAYER_W * 0.5f, 0.10f);
	fd.shape = &box;
	body->CreateFixture(&fd);

	b2CircleShape c;
	c.m_radius = PLAYER_R;
	c.m_p.Set(0.f, 0.10f);
	fd.shape = &c;
	body->CreateFixture(&fd);
	c.m_p.Set(0.f, -0.10f);
	body->CreateFixture(&fd);

	b2MassData md;
	md.mass = PLAYER_MASS;
	md.center.SetZero();
	md.I = 1.f;
	body->SetMassData(&md);
	state = PState::Air;
}

// ---------------------------------------------------------------------------

void Player::senseGround(World& world) {
	wasGrounded = grounded;
	grounded = false;
	groundBody = nullptr;
	onOneWay = false;
	onWood = false;
	groundNormal.Set(0.f, -1.f);
	if (state == PState::Dead || state == PState::Rope) return;

	b2Vec2 p = body->GetPosition();
	b2Vec2 v = body->GetLinearVelocity();
	b2PolygonShape foot;
	b2Vec2 fc(p.x, p.y + PLAYER_H * 0.5f + 0.02f);
	foot.SetAsBox(0.15f, 0.05f, fc, 0.f);
	b2Transform ident;
	ident.SetIdentity();

	QueryAll q;
	b2AABB box;
	box.lowerBound = fc - b2Vec2(0.16f, 0.06f);
	box.upperBound = fc + b2Vec2(0.16f, 0.06f);
	world.b2->QueryAABB(&q, box);

	int bestPriority = -1;
	for (b2Fixture* f : q.hits) {
		b2Body* ob = f->GetBody();
		if (ob == body || f->IsSensor()) continue;
		const b2Filter& fl = f->GetFilterData();
		if (!(fl.categoryBits & (CAT_TERRAIN | CAT_OBJECT)) || !(fl.maskBits & CAT_PLAYER)) continue;
		Entity* e = entityOf(ob);
		if (!e) continue;
		if (e->kind == Kind::OneWay) {
			float top = f->GetAABB(0).lowerBound.y;
			if (dropThrough > 0.f || bottom() > top + 0.06f || v.y < -0.5f) continue;
		}
		bool hit = false;
		for (int ci = 0; ci < f->GetShape()->GetChildCount() && !hit; ++ci)
			hit = b2TestOverlap(&foot, 0, f->GetShape(), ci, ident, ob->GetTransform());
		if (!hit) continue;
		// moving support must not register while we fly away from it
		b2Vec2 gv = ob->GetLinearVelocityFromWorldPoint(fc);
		if (sinceJump < 0.1f && v.y < gv.y - 1.f) continue;
		grounded = true;
		int prio = ob->GetType() == b2_staticBody ? 0 : 1;
		if (prio > bestPriority) {
			bestPriority = prio;
			groundBody = ob;
		}
		if (e->kind == Kind::OneWay) onOneWay = true;
		if (isWoodKind(e->kind)) onWood = true;
	}

	// Contact normals: ground normal for slopes and ceiling bumps.
	float bestNy = -0.5f;
	for (b2ContactEdge* ce = body->GetContactList(); ce; ce = ce->next) {
		b2Contact* c = ce->contact;
		if (!c->IsTouching() || !c->IsEnabled()) continue;
		if (c->GetFixtureA()->IsSensor() || c->GetFixtureB()->IsSensor()) continue;
		b2WorldManifold wm;
		c->GetWorldManifold(&wm);
		b2Vec2 n = wm.normal;
		if (c->GetFixtureA()->GetBody() == body) n = -n;
		if (grounded && n.y < bestNy) {
			bestNy = n.y;
			groundNormal = n;
		}
		if (n.y > 0.7f && preVy < -0.5f && (state == PState::Air)) {
			// head bump: no sticking to the ceiling
			b2Vec2 vv = body->GetLinearVelocity();
			if (vv.y < HEAD_BUMP_VY) {
				vv.y = HEAD_BUMP_VY;
				body->SetLinearVelocity(vv);
			}
			jumpCutAvailable = false;
		}
	}
	if (groundNormal.y > -0.3f) groundNormal.Set(0.f, -1.f);

	// water
	float top = p.y - PLAYER_H * 0.5f;
	int wet = 0;
	const float samples[3] = {top + 0.1f, p.y, p.y + PLAYER_H * 0.5f - 0.1f};
	for (float sy : samples)
		if (world.waterAtM(p.x, sy)) ++wet;
	submerged = wet / 3.f;
	if (wet) surfaceY = world.waterSurfaceAt(p.x, samples[2]);
}

// ---------------------------------------------------------------------------

void Player::doJump(World& world, float vy, float pitch) {
	b2Vec2 v = body->GetLinearVelocity();
	v.y = launch(vy);
	body->SetLinearVelocity(v);
	state = PState::Air;
	grounded = false;
	coyote = 0.f;
	jumpBuffer = 0.f;
	sinceJump = 0.f;
	jumpCutAvailable = true;
	b2Vec2 p = body->GetPosition();
	world.emit(Ev::Jump, p.x, bottom(), pitch);
}

void Player::releaseRope(World& world, bool hop, bool dropOnly) {
	if (ropeJoint) world.b2->DestroyJoint(ropeJoint);
	if (ropeLimit) world.b2->DestroyJoint(ropeLimit);
	ropeJoint = ropeLimit = nullptr;
	ignoredRope = rope;
	rope = nullptr;
	ropeCooldown = ROPE_COOLDOWN;
	state = PState::Air;
	spriteAngle = 0.f;
	b2Vec2 v = body->GetLinearVelocity();
	if (!dropOnly) v.x *= ROPE_RELEASE_VX;
	if (hop) v.y = std::min(v.y, launch(ROPE_RELEASE_VY));
	body->SetLinearVelocity(v);
	sinceJump = 0.f;
	jumpCutAvailable = false;
	b2Vec2 p = body->GetPosition();
	world.emit(Ev::RopeRelease, p.x, p.y);
	if (hop) world.emit(Ev::Jump, p.x, p.y, 1.1f);
}

static b2Joint* attachToSegment(World& world, Player& pl, Rope* rope, int seg, b2Vec2 anchorWorld) {
	b2RevoluteJointDef jd;
	jd.bodyA = rope->segs[seg];
	jd.bodyB = pl.body;
	jd.localAnchorA = rope->segs[seg]->GetLocalPoint(anchorWorld);
	jd.localAnchorB.SetZero();
	jd.collideConnected = false;
	b2Joint* j = world.b2->CreateJoint(&jd);
	// Rope-length limit anchor->player keeps the heavy monkey from stretching the light chain.
	if (pl.ropeLimit) world.b2->DestroyJoint(pl.ropeLimit);
	b2DistanceJointDef dd;
	dd.bodyA = world.anchor;
	dd.bodyB = pl.body;
	dd.localAnchorA = dd.bodyA->GetLocalPoint(rope->anchor);
	dd.localAnchorB.SetZero();
	float along = (seg + 0.5f) * ROPE_SEG_LEN;
	dd.maxLength = along * 1.02f;
	dd.minLength = 0.f;
	dd.length = dd.maxLength;
	dd.stiffness = 0.f;
	dd.damping = 0.f;
	pl.ropeLimit = world.b2->CreateJoint(&dd);
	return j;
}

void Player::tryGrabRope(World& world) {
	if (ropeCooldown > 0.f) return;
	b2Vec2 p = body->GetPosition();
	float best = 1e9f;
	Rope* bestRope = nullptr;
	int bestSeg = 0;
	bool overlapIgnored = false;
	for (auto& r : world.ropes) {
		for (size_t i = 0; i < r->segs.size(); ++i) {
			b2Vec2 s = r->segs[i]->GetPosition();
			float dx = std::fabs(s.x - p.x), dy = std::fabs(s.y - p.y);
			if (dx < PLAYER_W * 0.5f + 0.06f && dy < PLAYER_H * 0.5f + 0.1f) {
				if (r.get() == ignoredRope) {
					overlapIgnored = true;
					continue;
				}
				float d = dx * dx + dy * dy;
				if (d < best) {
					best = d;
					bestRope = r.get();
					bestSeg = static_cast<int>(i);
				}
			}
		}
	}
	if (!overlapIgnored) ignoredRope = nullptr;
	if (!bestRope) return;

	rope = bestRope;
	ropeSeg = std::max(0, bestSeg);
	b2Vec2 sc = rope->segs[ropeSeg]->GetPosition();
	b2Vec2 anchor(sc.x, std::clamp(p.y, sc.y - ROPE_SEG_LEN * 0.5f, sc.y + ROPE_SEG_LEN * 0.5f));
	ropeJoint = attachToSegment(world, *this, rope, ropeSeg, anchor);
	state = PState::Rope;
	climbTimer = 0.f;
	windVel = 0.f;
	world.emit(Ev::RopeGrab, p.x, p.y);
}

// ---------------------------------------------------------------------------

void Player::preStep(World& world, const Input& in, float dt) {
	b2Vec2 p = body->GetPosition();
	b2Vec2 v = body->GetLinearVelocity();
	preVy = v.y;

	if (state == PState::Dead) {
		deadTimer += dt;
		deathSpin += dt;
		return;
	}
	if (state == PState::Exiting) {
		exitTimer += dt;
		float dx = exitTargetX - p.x;
		v.x = std::clamp(dx * 8.f, -2.5f, 2.5f);
		body->SetLinearVelocity(v);
		fade = std::max(0.f, 1.f - exitTimer / 0.4f);
		if (std::fabs(v.x) > 0.3f) facing = sgn(v.x);
		updateAnim(dt);
		return;
	}

	inputDir = in.dir;
	lastJumpHeld = in.jumpHeld;
	if (in.dir != 0) facing = in.dir;
	if (in.jumpPressed) jumpBuffer = JUMP_BUFFER;
	else jumpBuffer = std::max(0.f, jumpBuffer - dt);
	sinceJump += dt;
	coyote = std::max(0.f, coyote - dt);
	dropThrough = std::max(0.f, dropThrough - dt);
	ropeCooldown = std::max(0.f, ropeCooldown - dt);
	swimCooldown = std::max(0.f, swimCooldown - dt);
	squashTimer = std::max(0.f, squashTimer - dt);
	bounceWindow = std::max(0.f, bounceWindow - dt);

	const bool windOn = world.windActive();
	uint8_t windCode = windOn ? world.windAtM(p.x, p.y) : static_cast<uint8_t>(WI_NONE);
	int windDir = windCode == WI_LEFT ? -1 : windCode == WI_RIGHT ? 1 : 0;
	// Updrafts lift while any of Pip is in the column (sampled at centre and feet), so he leaves
	// the top of the column with his feet and coasts ~0.6 tile above it (DESIGN 3.13).
	inUpdraft = windCode == WI_UP || (windOn && world.windAtM(p.x, p.y + PLAYER_H * 0.5f - 0.05f) == WI_UP);
	inWind = windDir != 0;

	// --- water transitions ---
	if ((state == PState::Ground || state == PState::Air) && submerged >= 0.5f && swimCooldown <= 0.f) {
		if (preVy > 4.f) world.emit(Ev::Splash, p.x, surfaceY, preVy);
		state = PState::Swim;
	} else if (state == PState::Swim && submerged < 0.34f) {
		state = grounded ? PState::Ground : PState::Air;
		world.emit(Ev::Drip, p.x, p.y);
	}

	// --- vine ---
	if (state == PState::Air && !grounded) tryGrabRope(world);
	else if (state != PState::Rope && ignoredRope) {
		// forget the just-released vine once Pip no longer overlaps it
		bool overlap = false;
		for (b2Body* sg : ignoredRope->segs) {
			b2Vec2 d = sg->GetPosition() - p;
			if (std::fabs(d.x) < PLAYER_W * 0.5f + 0.06f && std::fabs(d.y) < PLAYER_H * 0.5f + 0.1f) overlap = true;
		}
		if (!overlap) ignoredRope = nullptr;
	}

	switch (state) {
	case PState::Rope: {
		if (!rope) {
			state = PState::Air;
			break;
		}
		int last = static_cast<int>(rope->segs.size()) - 1;
		bool dropOff = in.down && ropeSeg >= last && !in.jumpPressed;
		if (in.jumpPressed || dropOff) {
			bool hop = in.jumpPressed && !in.down;
			releaseRope(world, hop, !hop);
			break;
		}
		if (in.dir != 0) body->ApplyForceToCenter(b2Vec2(ROPE_PUMP_FORCE * in.dir, 0.f), true);
		if (windDir != 0) body->ApplyForceToCenter(b2Vec2(WIND_AIR * PLAYER_MASS * windDir, 0.f), true);
		climbTimer = std::max(0.f, climbTimer - dt);
		int want = in.up ? -1 : (in.down ? 1 : 0);
		if (want != 0 && climbTimer <= 0.f) {
			int ns = std::clamp(ropeSeg + want, 0, last);
			if (ns != ropeSeg) {
				ropeSeg = ns;
				world.b2->DestroyJoint(ropeJoint);
				ropeJoint = attachToSegment(world, *this, rope, ropeSeg, rope->segs[ropeSeg]->GetPosition());
				climbTimer = ROPE_CLIMB_INTERVAL;
				world.emit(Ev::ClimbTick, p.x, p.y);
			}
		}
		spriteAngle = rope->segs[ropeSeg]->GetAngle();
		break;
	}
	case PState::Swim: {
		body->ApplyForceToCenter(b2Vec2(0.f, -(1.f - SWIM_GRAVITY_SCALE) * GRAVITY * PLAYER_MASS), true);
		// Currents push with 3 m/s^2 on top of Pip's own paddling (kept as a drift velocity,
		// like wind, so the velocity-driven control does not cancel it).
		uint8_t w = world.waterAtM(p.x, p.y);
		int flowDir = w == W_LEFT ? -1 : w == W_RIGHT ? 1 : 0;
		float ctrl = v.x - windVel;
		ctrl = approach(ctrl, in.dir * SWIM_MAX_VX, SWIM_ACCEL * dt);
		windVel += flowDir * CURRENT_FORCE / PLAYER_MASS * dt;
		windVel -= windVel * std::min(1.f, (flowDir ? 1.5f : 3.f) * dt);
		v.x = ctrl + windVel;
		float restY = surfaceY + 0.25f;
		if (in.up) v.y = -SWIM_VY;
		else if (in.down) v.y = SWIM_VY;
		else v.y = std::clamp((restY - p.y) * 6.f, -2.f, 2.f);
		if (jumpBuffer > 0.f && p.y - surfaceY < 0.4f) {
			body->SetLinearVelocity(v);
			doJump(world, SWIM_HOP_VY, 1.f);
			windVel = 0.f;
			jumpCutAvailable = false;
			swimCooldown = 0.25f;
			break;
		}
		body->SetLinearVelocity(v);
		break;
	}
	case PState::Ground:
	case PState::Air: {
		if (grounded) {
			state = PState::Ground;
			coyote = COYOTE;
		} else if (state == PState::Ground) {
			state = PState::Air;
		}

		// carrier velocity (movers, lifts, logs...)
		b2Vec2 carrier(0.f, 0.f);
		bool kinematicCarrier = false;
		if (grounded && groundBody && groundBody->GetType() != b2_staticBody) {
			carrier = groundBody->GetLinearVelocityFromWorldPoint(b2Vec2(p.x, bottom()));
			kinematicCarrier = groundBody->GetType() == b2_kinematicBody;
		}

		// drop through one-way branches
		if (grounded && onOneWay && in.down) {
			downOnOneWay += dt;
			if (downOnOneWay >= 0.15f) {
				dropThrough = 0.25f;
				downOnOneWay = 0.f;
				grounded = false;
				state = PState::Air;
			}
		} else {
			downOnOneWay = 0.f;
		}

		// jump
		if (jumpBuffer > 0.f && (grounded || coyote > 0.f) && dropThrough <= 0.f) {
			v.y = JUMP_VY;
			body->SetLinearVelocity(v);
			doJump(world, JUMP_VY + std::min(0.f, carrier.y * 0.5f), 1.f);
			v = body->GetLinearVelocity();
			grounded = false;
			kinematicCarrier = false;
		}
		// mushroom: a late press upgrades the bounce
		if (bounceWindow > 0.f && in.jumpPressed && v.y < 0.f) {
			v.y = launch(MUSHROOM_BIG_VY);
			bounceWindow = 0.f;
			jumpBuffer = 0.f;
		}
		// variable height
		if (state == PState::Air && jumpCutAvailable && !in.jumpHeld && v.y < 0.f &&
		    sinceJump >= JUMP_CUT_MIN_T) {
			v.y *= JUMP_CUT;
			jumpCutAvailable = false;
		}
		if (v.y >= 0.f) jumpCutAvailable = false;

		// horizontal control, relative to the carrier and excluding wind drift. Dynamic carriers
		// (logs, planks, crates) only lend their velocity while Pip stands still: running on them
		// in the world frame avoids a feedback loop where Pip's push speeds up a light log.
		if (!kinematicCarrier && in.dir != 0) carrier.x = 0.f;
		float vx = v.x - carrier.x - windVel;
		// On a slope the controlled speed is the speed along the surface, not its x component.
		b2Vec2 slopeT(0.f, 0.f);
		{
			Entity* ge = groundBody ? entityOf(groundBody) : nullptr;
			bool slopeGround = groundBody && (groundBody->GetType() == b2_staticBody || (ge && ge->kind == Kind::Seesaw));
			if (grounded && sinceJump > 0.05f && slopeGround && std::fabs(groundNormal.x) > 0.05f) {
				slopeT.Set(-groundNormal.y, groundNormal.x);
				if (slopeT.x < 0) slopeT = -slopeT;
				vx = b2Dot(v - carrier, slopeT) - windVel;
			}
		}
		float target = in.dir * RUN_SPEED;
		bool braking = false;
		if (grounded) {
			if (in.dir == 0) vx = approach(vx, 0.f, GROUND_DECEL * dt);
			else if (sgn(vx) == -in.dir && std::fabs(vx) > 0.01f) {
				vx = approach(vx, target, TURN_BRAKE * dt);
				braking = true;
			} else if (std::fabs(vx) > RUN_SPEED) vx = approach(vx, target, GROUND_DECEL * dt);
			else vx = approach(vx, target, GROUND_ACCEL * dt);
		} else {
			if (in.dir == 0) vx = approach(vx, 0.f, AIR_DECEL * dt);
			else if (vx * in.dir < RUN_SPEED) vx = approach(vx, target, AIR_ACCEL * dt);
		}
		(void)braking;

		// wind drift (3.13): accumulates while in wind, decays outside
		if (windDir != 0 && grounded && in.dir != 0) {
			// running on the ground: Pip's feet win, drift bleeds off quickly
			windVel -= windVel * std::min(1.f, 8.f * dt);
		} else if (windDir != 0) {
			windVel += windDir * (grounded ? WIND_GROUND : WIND_AIR) * dt;
			windVel -= windVel * (grounded ? 2.f : 0.35f) * dt;
		} else {
			windVel -= windVel * std::min(1.f, (grounded ? 8.f : 1.5f) * dt);
		}

		float outVx = vx + windVel + carrier.x;
		float outVy = v.y;

		if (grounded && sinceJump > 0.05f) {
			// Slope handling for terrain slopes and see-saw planks; wobbly bridges, logs and
			// crates just take horizontal velocity and let the contacts do the rest.
			bool tilted = slopeT.x > 0.f;
			if (tilted) {
				// run along the surface at full speed; idle = stand still
				b2Vec2 t = slopeT;
				float speed = vx + windVel;
				outVx = t.x * speed + carrier.x;
				outVy = t.y * speed + carrier.y;
				// cancel the tangential part of gravity so Pip does not slide
				b2Vec2 g(0.f, GRAVITY * PLAYER_MASS);
				float gt = b2Dot(g, t);
				body->ApplyForceToCenter(-gt * t, true);
			} else if (kinematicCarrier) {
				outVy = carrier.y;
			}
		}

		// extra gravity while falling, fall speed cap
		if (!grounded && outVy > 0.f)
			body->ApplyForceToCenter(b2Vec2(0.f, FALL_GRAVITY_EXTRA * PLAYER_MASS), true);
		if (inUpdraft && outVy > UPDRAFT_MAX_RISE)
			body->ApplyForceToCenter(b2Vec2(0.f, -UPDRAFT_ACCEL * PLAYER_MASS), true);
		outVy = std::min(outVy, MAX_FALL);

		// corner correction for head bumps (2.2)
		if (!grounded && outVy < 0.f) {
			float nextTop = p.y - PLAYER_H * 0.5f + outVy * dt - 0.01f;
			int row = static_cast<int>(std::floor(nextTop / TILE));
			float l = p.x - PLAYER_W * 0.5f, r = p.x + PLAYER_W * 0.5f;
			int c0 = static_cast<int>(std::floor(l / TILE)), c1 = static_cast<int>(std::floor(r / TILE));
			float overL = 0.f, overR = 0.f;
			bool mid = false;
			for (int c = c0; c <= c1; ++c) {
				if (!world.level.solid(c, row)) continue;
				float tl = c * TILE, tr = tl + TILE;
				if (tl <= l + 0.001f && tr >= r - 0.001f) mid = true;
				else if (tl <= l) overL = std::max(overL, tr - l);
				else if (tr >= r) overR = std::max(overR, r - tl);
				else mid = true;
			}
			if (!mid && (overL > 0.f) != (overR > 0.f)) {
				float shift = overL > 0.f ? overL : -overR;
				if (std::fabs(shift) <= CORNER_CORRECT) {
					b2Vec2 np(p.x + shift + (shift > 0 ? 0.005f : -0.005f), p.y);
					body->SetTransform(np, 0.f);
				}
			}
		}

		body->SetLinearVelocity(b2Vec2(outVx, outVy));
		if (!grounded) airVy = outVy;

		// footsteps
		if (grounded && std::fabs(vx) > 0.5f) {
			stepTimer -= dt;
			if (stepTimer <= 0.f) {
				stepTimer = onWood ? 0.22f : 0.14f;
				world.emit(Ev::Step, p.x, bottom(), onWood ? 1.f : 0.f);
			}
		} else {
			stepTimer = 0.f;
		}
		animTimer += dt * (grounded ? std::clamp(std::fabs(vx) / RUN_SPEED, 0.4f, 1.3f) : 1.f);
		break;
	}
	default:
		break;
	}
	updateAnim(dt);
}

// ---------------------------------------------------------------------------

void Player::postStep(World& world, float dt) {
	(void)dt;
	if (state == PState::Dead) return;
	senseGround(world);
	if (state == PState::Exiting) return;

	b2Vec2 p = body->GetPosition();
	b2Vec2 v = body->GetLinearVelocity();

	// landing
	if (grounded && !wasGrounded && (state == PState::Air || state == PState::Ground)) {
		Entity* ge = groundBody ? entityOf(groundBody) : nullptr;
		if (ge && ge->kind == Kind::Mushroom && airVy > 1.f) {
			Mushroom* m = static_cast<Mushroom*>(ge);
			bool big = jumpBuffer > 0.f || lastJumpHeld;
			v.y = launch(big ? MUSHROOM_BIG_VY : MUSHROOM_VY);
			body->SetLinearVelocity(v);
			bounceWindow = big ? 0.f : MUSHROOM_WINDOW;
			jumpBuffer = 0.f;
			grounded = false;
			state = PState::Air;
			sinceJump = 0.f;
			jumpCutAvailable = false;
			m->squash = 1.f;
			world.emit(Ev::Bounce, p.x, bottom(), big ? 1.25f : 1.f);
		} else if (airVy > HARD_LAND_VY) {
			squashTimer = 0.08f;
			world.emit(Ev::LandHard, p.x, bottom(), airVy);
		} else if (airVy > 1.5f) {
			world.emit(Ev::LandSoft, p.x, bottom(), airVy);
		}
		// crumbling ledge trigger
	}
	if (grounded && groundBody) {
		Entity* ge = entityOf(groundBody);
		if (ge && ge->kind == Kind::Crumble) {
			Crumble* c = static_cast<Crumble*>(ge);
			if (c->state == Crumble::Solid) {
				c->state = Crumble::Shaking;
				c->timer = 0.f;
				world.emit(Ev::CrumbleShake, c->tx * TILE + 0.25f, c->ty * TILE + 0.25f);
			}
		}
		if (ge && ge->kind == Kind::FallLog) {
			FallLog* fl = static_cast<FallLog*>(ge);
			if (fl->state == FallLog::Hanging) {
				fl->state = FallLog::Triggered;
				fl->timer = 0.f;
				world.emit(Ev::LogCreak, groundBody->GetPosition().x, groundBody->GetPosition().y);
			}
		}
	}
	// Other crumbles we are standing on (the foot sensor may touch two tiles)
	if (grounded) {
		for (auto& c : world.crumbles) {
			if (c->state != Crumble::Solid) continue;
			float cx = c->tx * TILE + 0.25f, cy = c->ty * TILE;
			if (std::fabs(cx - p.x) < 0.25f + 0.15f && std::fabs(cy - bottom()) < 0.06f) {
				c->state = Crumble::Shaking;
				c->timer = 0.f;
				world.emit(Ev::CrumbleShake, cx, cy + 0.25f);
			}
		}
	}

	// hazards: beetles, boulders, falling logs
	float pl = p.x - PLAYER_W * 0.5f, pr = p.x + PLAYER_W * 0.5f;
	float pt = p.y - PLAYER_H * 0.5f, pb = p.y + PLAYER_H * 0.5f;
	for (auto& b : world.beetles) {
		if (b->dead || !b->body) continue;
		b2Vec2 bp = b->body->GetPosition();
		if (pr < bp.x - 0.2f || pl > bp.x + 0.2f || pb < bp.y - 0.125f || pt > bp.y + 0.125f) continue;
		bool falling = !grounded && std::max(preVy, v.y) > 0.f;
		bool stomp = falling && pb < bp.y + 0.08f;
		if (stomp) {
			b->dead = true;
			b2Filter f;
			f.categoryBits = CAT_NONE;
			f.maskBits = 0;
			for (b2Fixture* fx = b->body->GetFixtureList(); fx; fx = fx->GetNext()) fx->SetFilterData(f);
			b->body->SetLinearVelocity(b2Vec2(0.f, -3.f));
			b2Vec2 nv = body->GetLinearVelocity();
			nv.y = launch(-6.f);
			body->SetLinearVelocity(nv);
			state = PState::Air;
			sinceJump = 0.f;
			jumpCutAvailable = false;
			world.emit(Ev::Squish, bp.x, bp.y);
		} else {
			// side contact: shrink the lethal box a little to be generous
			if (pr > bp.x - 0.15f && pl < bp.x + 0.15f) {
				kill(world);
				return;
			}
		}
	}
	for (b2ContactEdge* ce = body->GetContactList(); ce; ce = ce->next) {
		b2Contact* c = ce->contact;
		if (!c->IsTouching() || !c->IsEnabled()) continue;
		Entity* e = entityOf(ce->other);
		if (!e) continue;
		if (e->kind == Kind::Boulder) {
			b2Vec2 bv = static_cast<Boulder*>(e)->lastVel; // velocity before this step's impact
			b2Vec2 d = p - ce->other->GetPosition();
			d.Normalize();
			if (b2Dot(bv, d) > 3.f) {
				kill(world);
				return;
			}
		}
		if (e->kind == Kind::FallLog) {
			// Crushed only when the log comes down on top of Pip: contact normal pointing down onto
			// him and his centre under the log's span (a clipped shoulder just gets shoved).
			FallLog* lg = static_cast<FallLog*>(e);
			float lvy = std::max(lg->lastVy, ce->other->GetLinearVelocity().y);
			b2WorldManifold wm;
			c->GetWorldManifold(&wm);
			b2Vec2 n = c->GetFixtureA()->GetBody() == body ? -wm.normal : wm.normal; // log -> Pip
			float halfW = lg->tiles * TILE * 0.5f;
			bool under = std::fabs(p.x - ce->other->GetPosition().x) < halfW;
			if (lvy > 3.f && n.y > 0.6f && under && ce->other->GetPosition().y < p.y - 0.2f) {
				kill(world);
				return;
			}
		}
	}
}

void Player::kill(World& world) {
	if (state == PState::Dead) return;
	if (rope) {
		if (ropeJoint) world.b2->DestroyJoint(ropeJoint);
		if (ropeLimit) world.b2->DestroyJoint(ropeLimit);
		ropeJoint = ropeLimit = nullptr;
		rope = nullptr;
	}
	state = PState::Dead;
	++world.deaths;
	deadTimer = 0.f;
	deathSpin = 0.f;
	body->SetLinearVelocity(b2Vec2(0.f, 0.f));
	body->SetEnabled(false);
	b2Vec2 p = body->GetPosition();
	world.emit(Ev::Death, p.x, p.y);
}

void Player::updateAnim(float dt) {
	(void)dt;
	switch (state) {
	case PState::Air:
	case PState::Rope:
		anim = PAnim::Jump;
		break;
	case PState::Swim:
		swimPhase += dt;
		anim = std::fmod(swimPhase, 0.7f) < 0.35f ? PAnim::Walk1 : PAnim::Walk2;
		break;
	case PState::Ground:
	case PState::Exiting: {
		b2Vec2 v = body->GetLinearVelocity();
		float rel = v.x;
		if (groundBody && groundBody->GetType() != b2_staticBody) rel -= groundBody->GetLinearVelocity().x;
		bool braking = inputDir != 0 && sgn(rel) == -inputDir && std::fabs(rel) > 1.f;
		if (state == PState::Exiting) {
			animTimer += dt;
		}
		if (std::fabs(rel) > 0.3f && !braking) anim = std::fmod(animTimer, 0.24f) < 0.12f ? PAnim::Walk1 : PAnim::Walk2;
		else anim = PAnim::Stand;
		break;
	}
	default:
		break;
	}
	if (state != PState::Rope) spriteAngle *= 0.8f;
}
