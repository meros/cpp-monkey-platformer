/*
 * Rope.h
 *
 */

#ifndef ROPE_H_
#define ROPE_H_

#include "IDrawable.h"
#include "ContactListener.h"
#include "Sprite.h"

#include <SFML/Graphics.hpp>
#include <Box2D/Box2D.h>
#include <vector>

//A non interactive game block
class Rope
	: public IDrawable
	, public UserData
{
public:
	Rope(
		b2World&			aWorld,
		float 				aX,
		float 				aY);

	virtual 			~Rope();

	void				Draw(
		sf::RenderTarget&	aTarget,
		float				aXScale,
		float				aYScale,
		float 				aXTranslate,
		float 				aYTranslate);

	void				Update();

protected:
	void 				BeginContact(
							b2Contact* 			contact);

	void				PreSolve(
							b2Contact*			contact, 
							const b2Manifold*	oldManifold);
private:

        std::vector<b2Body*>	myParts;
        std::vector<b2RevoluteJoint*>	myJoints;

	Sprite		mySprite;
	b2World&	myWorld;
};

#endif ROPE_H_
