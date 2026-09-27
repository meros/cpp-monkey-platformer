// Leafwind -- Pip, the player controller (docs/DESIGN.md 2.2-2.5).
#pragma once

#include "Entities.h"
#include "Input.h"

enum class PState { Ground, Air, Rope, Swim, Dead, Exiting };

enum class PAnim { Stand, Walk1, Walk2, Jump };

class Player : public Entity {
public:
	Player() : Entity(Kind::Player) {}

	void create(b2World& w, b2Vec2 bottomCentre);
	void preStep(World& world, const Input& in, float dt);
	void postStep(World& world, float dt);

	b2Vec2 pos() const { return body->GetPosition(); }
	b2Vec2 vel() const { return body->GetLinearVelocity(); }
	float bottom() const { return body->GetPosition().y + 0.30f; }
	bool alive() const { return state != PState::Dead; }

	void kill(World& world);
	void releaseRope(World& world, bool hop, bool dropOnly);

	PState state = PState::Air;
	int facing = 1;
	int inputDir = 0;

	// ground sensing (results of the last postStep)
	bool grounded = false;
	bool wasGrounded = false;
	b2Body* groundBody = nullptr;
	b2Vec2 groundNormal{0, -1};
	bool onOneWay = false;
	bool onWood = false;
	float submerged = 0.f;
	float surfaceY = 0.f;

	// timers
	float coyote = 0.f;
	float jumpBuffer = 0.f;
	float sinceJump = 10.f;
	bool jumpCutAvailable = false;
	float dropThrough = 0.f;
	float downOnOneWay = 0.f;
	float bounceWindow = 0.f;
	float ropeCooldown = 0.f;
	float climbTimer = 0.f;
	float airVy = 0.f;       // last vy while airborne (landing impact)
	float preVy = 0.f;       // vy before the physics step
	float windVel = 0.f;     // accumulated wind velocity (3.13)
	float stepTimer = 0.f;
	float exitTimer = 0.f;
	float exitTargetX = 0.f;
	float deadTimer = 0.f;
	float swimCooldown = 0.f;
	bool inUpdraft = false;
	bool inWind = false;
	bool lastJumpHeld = false;

	// vine
	Rope* rope = nullptr;
	int ropeSeg = 0;
	b2Joint* ropeJoint = nullptr;
	b2Joint* ropeLimit = nullptr;
	Rope* ignoredRope = nullptr;

	// animation
	PAnim anim = PAnim::Stand;
	float animTimer = 0.f;
	float spriteAngle = 0.f;   // radians, on vine
	float squashTimer = 0.f;   // landing squash
	float squashAmt = 1.f;     // 1 = hard landing, less for a soft one
	float stretchTimer = 0.f;  // take-off / bounce stretch
	float swimPhase = 0.f;
	float deathSpin = 0.f;
	float fade = 1.f;          // exit fade

private:
	void senseGround(World& world);
	void updateAnim(float dt);
	void tryGrabRope(World& world);
	void doJump(World& world, float vy, float pitch);
};
