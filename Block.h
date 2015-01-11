/*
 * Block.h
 *
 *  Created on: 8 maj 2010
 *      Author: meros
 */

#ifndef BLOCK_H_
#define BLOCK_H_

#include "IDrawable.h"
#include "ContactListener.h"
#include "Sprite.h"

#include <Box2D/Box2D.h>

//A non interactive game block
class Block: public IDrawable, public UserData {
public:
	Block(b2World& aWorld, float aX, float aY, float aW, float aH,
			bool aDynamic = false);

	virtual ~Block();

	void Draw(sf::RenderTarget& aTarget, float aXScale, float aYScale,
			float aXTranslate, float aYTranslate);

	void SetCloud(bool aCloudFlag);

protected:
	void PreSolve(b2Contact* contact, const b2Manifold* oldManifold);
private:
	float myX;
	float myY;
	float myW;
	float myH;

	b2Body* myCollisionBody;
	Sprite mySprite;

	bool myIsCloud;
};

#endif /* BLOCK_H_ */
