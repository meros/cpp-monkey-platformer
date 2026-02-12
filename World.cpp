/*
 * World.cpp
 *
 *  Created on: 10 maj 2010
 *      Author: meros
 */

#include "World.h"

#include <fstream>

using namespace std;

#define BLOCK_SIZE 0.20

World::World() :
		myCollisionWorld(b2Vec2(0.0f, 20.0f)), myFrameCounter(0) {
	// TODO Auto-generated constructor stub

	ifstream level("data/level.txt");

	char buf[1024];

	int px = 0;
	int py = 0;

	int y = 0;
	while (level.getline(buf, 1024))

	{
		int x = 0;
		char* block = buf;

		int startpos = -1;
		while (*block) {
			if (*block == 'P') {
				px = x;
				py = y;
			}

			if (*block == 'X' && startpos == -1) {
				startpos = x;
			} else if (*block != 'X' && startpos != -1) {
				myBlocks.push_back(
						new Block(myCollisionWorld, startpos * BLOCK_SIZE,
								y * BLOCK_SIZE, BLOCK_SIZE * (x - startpos),
								BLOCK_SIZE));
				startpos = -1;
			}

			//Create rope
			if (*block == 'R') {
				myRopes.push_back(
						new Rope(myCollisionWorld, (x + 0.5f) * BLOCK_SIZE,
								y * BLOCK_SIZE));
			}

			//Create bridge
			if (*block == 'B') {
				myBridges.push_back(
						new Bridge(myCollisionWorld, x * BLOCK_SIZE,
								y * BLOCK_SIZE));
			}

			block++;
			x++;
		}

		if (startpos != -1) {
			myBlocks.push_back(
					new Block(myCollisionWorld, startpos * BLOCK_SIZE,
							y * BLOCK_SIZE, BLOCK_SIZE * (x - startpos),
							BLOCK_SIZE));
		}

		startpos = -1;
		y++;
	}

	myPlayer = new Player(myCollisionWorld, (px + 0.5) * BLOCK_SIZE,
			(py + 0.5) * BLOCK_SIZE);

	myContactListener = new ContactListener(myCollisionWorld);
}

World::~World() {
}

Point World::GetPlayerPosition() const {
	return myPlayer->GetPosition();
}

void World::Update() {
	myPlayer->Update();

	vector<Rope*>::iterator it;
	for (it = myRopes.begin(); it != myRopes.end(); it++)
		(*it)->Update();

	myCollisionWorld.Step(1.0f / 50.0f, 500, 50);
}

void World::Draw(sf::RenderTarget& aTarget) {
	myFrameCounter++;

	float xscale = 120;
	float yscale = xscale;

	float xtrans = myPlayer->GetPosition().myX - (400 / xscale);
	float ytrans = myPlayer->GetPosition().myY - (300 / yscale);

	myPlayer->Draw(aTarget, xscale, yscale, xtrans, ytrans);

	{
		vector<Block*>::iterator it;
		for (it = myBlocks.begin(); it != myBlocks.end(); it++)
			(*it)->Draw(aTarget, xscale, yscale, xtrans, ytrans);
	}

	{
		vector<Rope*>::iterator it;
		for (it = myRopes.begin(); it != myRopes.end(); it++)
			(*it)->Draw(aTarget, xscale, yscale, xtrans, ytrans);
	}

	{
		vector<Bridge*>::iterator it;
		for (it = myBridges.begin(); it != myBridges.end(); it++)
			(*it)->Draw(aTarget, xscale, yscale, xtrans, ytrans);
	}

}
