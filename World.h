/*
 * World.h
 *
 *  Created on: 10 maj 2010
 *      Author: meros
 */

#ifndef WORLD_H_
#define WORLD_H_

#include "Block.h"
#include "Rope.h"
#include "Bridge.h"
#include "Player.h"

#include <vector>
#include <Box2D/Box2D.h>


class World {
public:
				World();
	virtual 	~World();

	void 		Update(
					const sf::Input& 	aInput);
	void 		Draw(
					sf::RenderTarget&	aTarget);
private:
	b2World 		myCollisionWorld;


	std::vector<Block*> 	myBlocks;
        std::vector<Rope*> 	myRopes;
        std::vector<Bridge*> 	myBridges;
	Player* 		myPlayer;
	ContactListener* myContactListener;

	int myFrameCounter;
};

#endif /* WORLD_H_ */
