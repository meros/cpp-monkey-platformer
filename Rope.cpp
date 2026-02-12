/*
 * Rope.cpp
 *
 */

#include "Rope.h"


Rope::Rope(b2World& aWorld, float aX, float aY) :
		myWorld(aWorld) {
	b2Body* lastBody;
	for (int i = 0; i < 10; i++) {
		float ROPE_LENGTH = 0.2;
		float ROPE_WIDTH = 0.01;

		b2BodyDef dropboxDef2;
		dropboxDef2.position = b2Vec2(aX, aY + i * ROPE_LENGTH);
		dropboxDef2.linearDamping = 0.8f;
		//dropboxDef.angle = 0.2;

		if (i != 0) {
			dropboxDef2.type = b2_dynamicBody;
		}

		b2PolygonShape dropboxShape2;

		dropboxShape2.SetAsBox(ROPE_WIDTH / 2, ROPE_LENGTH / 2);

		b2Body* body2 = aWorld.CreateBody(&dropboxDef2);
		b2FixtureDef fixDef;
		fixDef.shape = &dropboxShape2;
		fixDef.density = 0.05f;
		fixDef.userData.pointer = reinterpret_cast<uintptr_t>(static_cast<UserData*>(this));
		body2->CreateFixture(&fixDef);

		if (i != 0) {
			b2RevoluteJointDef jointDef;
			b2Vec2 anchor = (lastBody->GetPosition() + body2->GetPosition());
			anchor *= 0.5;
			jointDef.Initialize(lastBody, body2, anchor);
			jointDef.collideConnected = false;
			jointDef.enableLimit = true;
			jointDef.upperAngle = 0.8;
			jointDef.lowerAngle = -0.8;

			b2RevoluteJoint* joint = (b2RevoluteJoint*) aWorld.CreateJoint(
					&jointDef);

			myJoints.push_back(joint);
		}

		myParts.push_back(body2);

		lastBody = body2;
	}

	mySprite.LoadTGA("data/rope.tga");
}

Rope::~Rope() {

}

void Rope::Draw(sf::RenderTarget& aTarget, float aXScale, float aYScale,
		float aXTranslate, float aYTranslate) {
	std::vector<b2Body*>::iterator it;
	for (it = myParts.begin(); it != myParts.end(); it++) {
		float x = ((*it)->GetPosition().x - aXTranslate) * aXScale;
		float y = ((*it)->GetPosition().y - aYTranslate) * aYScale;

		mySprite.DrawStretchRotation(aTarget, x, y, aYScale / 3,
				(*it)->GetAngle());
	}
}

void Rope::BeginContact(b2Contact* contact) {
	auto ptrA = contact->GetFixtureA()->GetUserData().pointer;
	auto ptrB = contact->GetFixtureB()->GetUserData().pointer;

	if (!ptrA || !ptrB)
		return;

	auto* udA = reinterpret_cast<UserData*>(ptrA);
	auto* udB = reinterpret_cast<UserData*>(ptrB);

	if (udA == static_cast<UserData*>(this)
			&& udB == static_cast<UserData*>(this))
		return;

	if (udA == static_cast<UserData*>(this)) {
		udB->TouchedRope(this, contact->GetFixtureA()->GetBody());
	}

	if (udB == static_cast<UserData*>(this)) {
		udA->TouchedRope(this, contact->GetFixtureB()->GetBody());
	}
}

void Rope::Update() {

}

void Rope::MoveJoint(b2Joint* joint, float aDeltaY) {
	// TODO: disconnect joint and move it!

}


void Rope::PreSolve(b2Contact* contact, const b2Manifold* oldManifold) {
	contact->SetEnabled(false);
}
