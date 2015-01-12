/*
 * Player.cpp
 *
 *  Created on: 4 maj 2010
 *      Author: meros
 */

#include "Player.h"
#include "Rope.h"

#define MAXFALLSPEED 5
#define JUMP_SPEED_BEG -4
#define JUMP_SPEED_END -3
#define JUMP_SPEED_STOP -2
#define JUMP_MAX_LEN 0.7

#define COLLISION_HEIGHT 0.26f
#define COLLISION_WIDTH	0.16f

#define RUN_BREAK_FACTOR 0.2
#define RUN_STOP_THRESHOLD 0.1
#define RUN_ACC 0.6
#define MAX_RUN_SPEED 3

#define MID_AIR_STEER_ACC 0.4

#define MID_AIR_SLOWDOWN_FACTOR 0.1
#define MID_AIR_STOP_THRESHOLD 0.01

#define ROPE_STEER_ACC 0.2

#define ROPE_SLOWDOWN_FACTOR 0

#include <iostream>
#include <cmath>

using namespace std;

Player::Player(b2World& aWorld, float aXStartPos, float aYStartPos) :
		myNewlyTouchedRopeBody(NULL), myNewlyTouchedRope(NULL), myRopeJoint(
				NULL), myRope(NULL), myWorld(aWorld), myIsBreaking(false), myTouchingRoofContacts(
				0), myJumpStartY(0.0f) {

	myType = UserData::Player;

	myDX = 0;
	myDY = 0;
	myAnimationCounter = 0;
	myTouchingGroundContacts = 0;

	myDirectionIsRight = true;

	myState = STATE_MidAir;
	myJumpBtnReleased = true;

	myStandRightSprite.LoadTGA("data/apa_stand_right.tga");
	myBreakRightSprite.LoadTGA("data/mario_break_right.tga");

	myWalk1RightSprite.LoadTGA("data/apa_walk_right_1.tga");
	myWalk2RightSprite.LoadTGA("data/apa_walk_right_2.tga");

	myJumpRightSprite.LoadTGA("data/apa_jump_right.tga");

	myStandLeftSprite.LoadTGA("data/apa_stand_left.tga");
	myBreakLeftSprite.LoadTGA("data/mario_break_left.tga");

	myClimbLeftSprite.LoadTGA("data/mario_jump_left.tga");
	myClimbRightSprite.LoadTGA("data/mario_jump_left.tga");
	myClimbRightSprite.Mirror();

	myWalk1LeftSprite.LoadTGA("data/apa_walk_right_1.tga");
	myWalk1LeftSprite.Mirror();
	myWalk2LeftSprite.LoadTGA("data/apa_walk_right_2.tga");
	myWalk2LeftSprite.Mirror();

	myJumpLeftSprite.LoadTGA("data/apa_jump_left.tga");

	// Set up collision box
	b2BodyDef dropboxDef;

	dropboxDef.type = b2_dynamicBody;
	dropboxDef.position = b2Vec2(aXStartPos, aYStartPos);

	b2PolygonShape dropboxShape;

	float roundof = 0.02;

	b2Vec2 verts[8];
	//	verts[0] = b2Vec2(-COLLISION_WIDTH/2, -COLLISION_HEIGHT/2);
	//verts[1] = b2Vec2(COLLISION_WIDTH/2, -COLLISION_HEIGHT/2);
	verts[0] = b2Vec2(-COLLISION_WIDTH / 2, -COLLISION_HEIGHT / 2 + roundof);
	verts[1] = b2Vec2(-COLLISION_WIDTH / 2 + roundof, -COLLISION_HEIGHT / 2);

	verts[2] = b2Vec2(COLLISION_WIDTH / 2 - roundof, -COLLISION_HEIGHT / 2);
	verts[3] = b2Vec2(COLLISION_WIDTH / 2, -COLLISION_HEIGHT / 2 + roundof);

	verts[4] = b2Vec2(COLLISION_WIDTH / 2, COLLISION_HEIGHT / 2 - roundof);
	verts[5] = b2Vec2(COLLISION_WIDTH / 2 - roundof, COLLISION_HEIGHT / 2);

	verts[6] = b2Vec2(-COLLISION_WIDTH / 2 + roundof, COLLISION_HEIGHT / 2);
	verts[7] = b2Vec2(-COLLISION_WIDTH / 2, COLLISION_HEIGHT / 2 - roundof);

	dropboxShape.Set(verts, 8);
	//dropboxShape.SetAsBox(COLLISION_WIDTH/2, COLLISION_HEIGHT/2);

	myCollisionBody = aWorld.CreateBody(&dropboxDef);
	myCollisionBody->SetFixedRotation(true);

	b2FixtureDef fixtureDef;

	fixtureDef.shape = &dropboxShape;
	fixtureDef.density = 0.1f;
	fixtureDef.friction = 0.4f;

	myCollisionBody->CreateFixture(&fixtureDef)->SetUserData((UserData*) this);
}

Player::~Player() {
	// TODO Auto-generated destructor stub
}

void Player::Update() {
	myDY = myCollisionBody->GetLinearVelocity().y;
	myDX = myCollisionBody->GetLinearVelocity().x;

	if (myState == STATE_MidAir && myNewlyTouchedRopeBody) {
		b2RevoluteJointDef jointDef;
		b2Vec2 anchor = (myNewlyTouchedRopeBody->GetPosition()
				+ myCollisionBody->GetPosition());
		anchor *= 0.5;
		jointDef.Initialize(myNewlyTouchedRopeBody, myCollisionBody, anchor);
		jointDef.collideConnected = false;

		myRope = myNewlyTouchedRope;
		myRopeJoint = myWorld.CreateJoint(&jointDef);

		myState = STATE_Rope;
	}

	if (PrivDoJump() && myState == STATE_Rope) {
		myWorld.DestroyJoint(myRopeJoint);
		myRopeJoint = NULL;
		myJumpBtnReleased = false;
		myState = STATE_MidJump;
		myDY = JUMP_SPEED_BEG;
		myJumpStartY = myCollisionBody->GetPosition().y;

		myState = STATE_MidJump;
	}

	myNewlyTouchedRopeBody = NULL;

	if (myTouchingGroundContacts > 0) {
		if (myState == STATE_MidAir) {
			myState = STATE_Ground;
		}
	} else {
		if (myDY > 0.1 && myState == STATE_Ground) {
			myState = STATE_MidAir;
		}
	}

	myCollisionBody->SetAwake(true);

	if (!PrivDoJump()) {
		myJumpBtnReleased = true;
	}

	myIsBreaking = false;

	//Swinging
	if (myState == STATE_Rope) {
		if (myDX > 0 && PrivDoRight()) {
			myDX += ROPE_STEER_ACC * (1 - (abs(myDX) / MAX_RUN_SPEED));
		} else if (myDX < 0 && PrivDoLeft()) {
			myDX -= ROPE_STEER_ACC * (1 - (abs(myDX) / MAX_RUN_SPEED));
		} else {
			//Mid air slowing down
			myDX -= myDX * ROPE_SLOWDOWN_FACTOR;
		}

		if (PrivDoUp()) {
			myDY = 1;
			myRope->MoveJoint(myRopeJoint, 1);
		} else if (PrivDoDown()) {
			myDY = -1;
			myRope->MoveJoint(myRopeJoint, -1);
		}
	}

	if (myState == STATE_MidAir || myState == STATE_MidJump) {
		//Mid air steering
		if (PrivDoRight()) {
			myDX += MID_AIR_STEER_ACC * (1 - (abs(myDX) / MAX_RUN_SPEED));
		} else if (PrivDoLeft()) {
			myDX -= MID_AIR_STEER_ACC * (1 - (abs(myDX) / MAX_RUN_SPEED));
		} else {
			//Mid air slowing down
			myDX -= myDX * MID_AIR_SLOWDOWN_FACTOR;
			if (abs(myDX) < MID_AIR_STOP_THRESHOLD) {
				myDX = 0;
			}
		}
	}

	//Falling maxfallspeed
	if (myDY > MAXFALLSPEED)
		myDY = MAXFALLSPEED;

	switch (myState) {
	case STATE_MidAir: {
		break;
	}
	case STATE_Ground:
		//Begin jump?
		if (PrivDoJump() && myJumpBtnReleased) {
			myJumpBtnReleased = false;
			myState = STATE_MidJump;
			myDY = JUMP_SPEED_BEG;
			myJumpStartY = myCollisionBody->GetPosition().y;
		} else {
			//Normal running
			if (PrivDoRight()) {
				if (myDX < -0.1) {
					myIsBreaking = true;
					myDX -= myDX * RUN_BREAK_FACTOR;
				} else
					myDX += RUN_ACC * (1 - (abs(myDX) / MAX_RUN_SPEED));
			} else if (PrivDoLeft()) {
				if (myDX > 0.1) {
					myIsBreaking = true;
					myDX -= myDX * RUN_BREAK_FACTOR;
				} else
					myDX -= RUN_ACC * (1 - (abs(myDX) / MAX_RUN_SPEED));
			}

			if (abs(myDX) < RUN_STOP_THRESHOLD) {
				myDX = 0;
			}
		}
		break;
	case STATE_MidJump:

		if (!PrivDoJump()
				|| (myJumpStartY - myCollisionBody->GetPosition().y)
						> JUMP_MAX_LEN || myTouchingRoofContacts != 0) {
			myState = STATE_MidAir;
			myDY = JUMP_SPEED_STOP;
		} else {
			myDY = ((JUMP_MAX_LEN
					- (myJumpStartY - myCollisionBody->GetPosition().y))
					/ JUMP_MAX_LEN) * JUMP_SPEED_BEG
					+ ((myJumpStartY - myCollisionBody->GetPosition().y)
							/ JUMP_MAX_LEN) * JUMP_SPEED_END;
		}
		break;
	case STATE_Rope:
		break;
	default:
		myDY = 0;
		break;
	}

	if (!sf::Keyboard::isKeyPressed(sf::Keyboard::A))
		myCollisionBody->SetLinearVelocity(b2Vec2(myDX, myDY));

	if (myDX > 0.1)
		myDirectionIsRight = true;

	if (myDX < -0.1)
		myDirectionIsRight = false;

	myTouchingGroundContacts = 0;
	myTouchingRoofContacts = 0;
}

void Player::Draw(sf::RenderTarget& aTarget, float aXScale, float aYScale,
		float aXTranslate, float aYTranslate) {
	myAnimationCounter++;

	float x = (myCollisionBody->GetPosition().x - aXTranslate) * aXScale;
	float y = (myCollisionBody->GetPosition().y - aYTranslate) * aYScale;

	//FIXME: matrix matrix matrix

	Sprite* spriteToDraw = NULL;

	if (myState == STATE_Ground) {
		if (myDirectionIsRight) {
			if (myIsBreaking) {
				spriteToDraw = &myBreakRightSprite;
			} else if (myDX < RUN_STOP_THRESHOLD) {
				spriteToDraw = &myStandRightSprite;
			} else {
				if ((myAnimationCounter / 4) % 2 == 0)
					spriteToDraw = &myWalk1RightSprite;
				else
					spriteToDraw = &myWalk2RightSprite;
			}
		} else {
			if (myIsBreaking) {
				spriteToDraw = &myBreakLeftSprite;
			} else if (-myDX < RUN_STOP_THRESHOLD) {
				spriteToDraw = &myStandLeftSprite;
			} else {
				if ((myAnimationCounter / 4) % 2 == 0)
					spriteToDraw = &myWalk1LeftSprite;
				else
					spriteToDraw = &myWalk2LeftSprite;
			}
		}
	} else if (myState == STATE_Rope) {
		if (myDirectionIsRight) {
			spriteToDraw = &myClimbRightSprite;
		} else {
			spriteToDraw = &myClimbLeftSprite;
		}
	} else {
		if (myDirectionIsRight) {
			spriteToDraw = &myJumpRightSprite;
		} else {
			spriteToDraw = &myJumpLeftSprite;
		}
	}

	if (spriteToDraw) {
		spriteToDraw->DrawGroundCenterRelative(aTarget, x,
				y + (COLLISION_HEIGHT * aYScale / 2.0f),
				COLLISION_HEIGHT * aYScale * 2,
				myCollisionBody->GetTransform().q.GetAngle());
	}
}

void Player::SetPosition(float aX, float aY) {

}

Point Player::GetPosition() {
	Point res;
	res.myX = myCollisionBody->GetPosition().x;
	res.myY = myCollisionBody->GetPosition().y;

	return res;
}

void Player::PostSolve(b2Contact* contact, const b2ContactImpulse* impulse) {
	b2WorldManifold manifold;
	contact->GetWorldManifold(&manifold);

	cout << endl;

	if (manifold.points[0].y
			>= myCollisionBody->GetPosition().y + (COLLISION_HEIGHT / 2) * 0.9
			&& abs(manifold.points[0].x - myCollisionBody->GetPosition().x)
					<= (COLLISION_HEIGHT / 2) * 0.95) {
		cout << "ground!" << endl;
		myTouchingGroundContacts++;
	}
	if (manifold.points[0].y
			<= myCollisionBody->GetPosition().y + -(COLLISION_HEIGHT / 2) * 0.9
			&& abs(manifold.points[0].x - myCollisionBody->GetPosition().x)
					<= (COLLISION_HEIGHT / 2) * 0.95) {
		myTouchingRoofContacts++;
	}
}

void Player::TouchedRope(Rope* aRope, b2Body* aBody) {
	myNewlyTouchedRopeBody = aBody;
	myNewlyTouchedRope = aRope;
}
