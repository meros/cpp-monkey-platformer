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

class Player
: public IDrawable
, public IPositionable
, public UserData
, public b2ContactFilter
{
public:
				Player(
						b2World& 				aWorld,
						float					aXStartPos,
						float					aYStartPos);

	virtual 	~Player();

	void 		Update(
					const sf::Input&			aInput);

	void 		Draw(
					sf::RenderTarget&	aTarget,
					float				aXScale,
					float				aYScale,
					float 				aXTranslate,
					float 				aYTranslate);

	void 		SetPosition(
					float 		aX,
					float 		aY);

	Point		GetPosition();

protected:
	void 		PostSolve(
			b2Contact* contact,
			const b2ContactImpulse* impulse);

	void		TouchedRope(
					Rope*		aRope, 
					b2Body*		aBpdy);

private:
	enum State
	{
		STATE_MidAir,
		STATE_MidJump,
		STATE_Ground,
	} myState;

	bool  myDirectionIsRight;
	bool  myIsBreaking;
	int   myTouchingGroundContacts;
	int   myTouchingRoofContacts;

	float myDX;
	float myDY;

	b2Body*	myCollisionBody;

	bool  	myJumpBtnReleased;
	float 	myJumpStartY;

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

	b2Body*		myTouchedRopeBody;
	b2Joint*	myRopeJoint;

	b2World&	myWorld;
	
	bool 		PrivDoJump(
					const sf::Input&    aInput) {return aInput.IsKeyDown(sf::Key::Up);}
	bool 		PrivDoRight(
					const sf::Input&    aInput) {return aInput.IsKeyDown(sf::Key::Right);}
	bool 		PrivDoLeft(
					const sf::Input&    aInput) {return aInput.IsKeyDown(sf::Key::Left);}
};

#endif /* PLAYER_H_ */
