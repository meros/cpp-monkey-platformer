/*
 * Block.cpp
 *
 *  Created on: 8 maj 2010
 *      Author: meros
 */

#include "Block.h"


Block::Block(b2World& aWorld, float aX, float aY, float aW, float aH,
		bool aDynamic) {
	myX = aX;
	myY = aY;
	myW = aW;
	myH = aH;

	myIsCloud = false;

	// Set up collision box
	b2BodyDef dropboxDef;
	dropboxDef.position = b2Vec2(aX + aW / 2, aY + aH / 2);
	//dropboxDef.angle = 0.2;

	if (aDynamic)
		dropboxDef.type = b2_dynamicBody;

	b2PolygonShape dropboxShape;

	dropboxShape.SetAsBox(aW / 2, aH / 2);

	myCollisionBody = aWorld.CreateBody(&dropboxDef);
	b2FixtureDef fixDef;
	fixDef.shape = &dropboxShape;
	fixDef.density = 0.05f;
	fixDef.userData.pointer = reinterpret_cast<uintptr_t>(static_cast<UserData*>(this));
	myCollisionBody->CreateFixture(&fixDef);

	mySprite.LoadTGA("data/groundtile.tga");
}

Block::~Block() {
	// TODO Auto-generated destructor stub
}

void Block::Draw(sf::RenderTarget& aTarget, float aXScale, float aYScale,
		float aXTranslate, float aYTranslate) {

	mySprite.DrawTiling(aTarget, (myX - aXTranslate) * aXScale,
			(myY - aYTranslate) * aYScale, (myX + myW - aXTranslate) * aXScale,
			(myY + myH - aYTranslate) * aYScale);

}

void Block::SetCloud(bool aCloudFlag) {
	myIsCloud = aCloudFlag;
}

void Block::PreSolve(b2Contact* contact, const b2Manifold* oldManifold) {
	if (myIsCloud) {
		b2Fixture* otherFix = contact->GetFixtureA();
		auto* udA = reinterpret_cast<UserData*>(contact->GetFixtureA()->GetUserData().pointer);
		if (udA == static_cast<UserData*>(this)) {
			otherFix = contact->GetFixtureB();
		}

		if (myCollisionBody->GetPosition().y
				< otherFix->GetAABB(0).upperBound.y)
			contact->SetEnabled(false);
	}
}
