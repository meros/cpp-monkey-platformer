/*
 * Player.h
 *
 *  Created on: 4 maj 2010
 *      Author: meros
 */

#ifndef PLAYER_H_
#define PLAYER_H_

#include "IDrawable.h"
#include "IPositionable.h"
#include "Sprite.h"
#include "ContactListener.h"

#include "DummyDrawable.h"

#include <Box2D/Box2D.h>

#define PLAYER_USER_DATA "Player"

#include <SFML/Window.hpp>

class Player: public IDrawable,
		public IPositionable,
		public UserData,
		public b2ContactFilter {
public:
	Player(b2World& aWorld, float aXStartPos, float aYStartPos);

	virtual ~Player();

	void Update();

	void Draw(sf::RenderTarget& aTarget, float aXScale, float aYScale,
			float aXTranslate, float aYTranslate);

	void SetPosition(float aX, float aY);

	Point GetPosition();

protected:
	void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse);

	void TouchedRope(Rope* aRope, b2Body* aBpdy);

private:
	enum State {
		STATE_MidAir, STATE_MidJump, STATE_Ground, STATE_Rope,
	} myState;

	bool myDirectionIsRight;
	bool myIsBreaking;
	int myTouchingGroundContacts;
	int myTouchingRoofContacts;

	float myDX;
	float myDY;

	b2Body* myCollisionBody;

	bool myJumpBtnReleased;
	float myJumpStartY;

	Sprite myClimbRightSprite;
	Sprite myClimbLeftSprite;

	Sprite myStandRightSprite;
	Sprite myBreakRightSprite;

	Sprite myWalk1RightSprite;
	Sprite myWalk2RightSprite;

	Sprite myJumpRightSprite;

	Sprite myStandLeftSprite;
	Sprite myBreakLeftSprite;

	Sprite myWalk1LeftSprite;
	Sprite myWalk2LeftSprite;

	Sprite myJumpLeftSprite;

	int myAnimationCounter;

	b2Body* myNewlyTouchedRopeBody;
	Rope* myNewlyTouchedRope;

	b2Joint* myRopeJoint;
	Rope* myRope;

	b2World& myWorld;

	bool PrivDoJump() {
		return sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
	}
	bool PrivDoRight() {
		return sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
	}
	bool PrivDoLeft() {
		return sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
	}
	bool PrivDoUp() {
		return sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
	}
	bool PrivDoDown() {
		return sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
	}
};

#endif /* PLAYER_H_ */
