/*
 * World.cpp
 *
 *  Created on: 10 maj 2010
 *      Author: meros
 */

#include "World.h"

#include <fstream>

using namespace std;

class debugdraw: public b2Draw {
public:
	/// Draw a closed polygon provided in CCW order.
	void DrawPolygon(const b2Vec2* vertices, int32 vertexCount,
			const b2Color& color) {
		for (int i = 0; i < vertexCount; i++) {
			float x1 = (vertices[i].x - myOffsetX) * myScaleX;
			float y1 = (vertices[i].y - myOffsetY) * myScaleY;
			float x2 = (vertices[(i + 1) % vertexCount].x - myOffsetX)
					* myScaleX;
			float y2 = (vertices[(i + 1) % vertexCount].y - myOffsetY)
					* myScaleY;

			sf::Vertex line[] = { sf::Vertex(sf::Vector2f(x1, y1)), sf::Vertex(
					sf::Vector2f(x2, y2)) };

			myBuffer->draw(line, 2, sf::Lines);
		}
	}

	/// Draw a solid closed polygon provided in CCW order.
	void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount,
			const b2Color& color) {
		for (int i = 0; i < vertexCount; i++) {
			float x1 = (vertices[i].x - myOffsetX) * myScaleX;
			float y1 = (vertices[i].y - myOffsetY) * myScaleY;
			float x2 = (vertices[(i + 1) % vertexCount].x - myOffsetX)
					* myScaleX;
			float y2 = (vertices[(i + 1) % vertexCount].y - myOffsetY)
					* myScaleY;

			sf::ConvexShape convex;
			convex.setPointCount(2);
			convex.setPoint(0, sf::Vector2f(x1, y1));
			convex.setPoint(1, sf::Vector2f(x2, y2));
			myBuffer->draw(convex);
		}
	}

	/// Draw a circle.
	void DrawCircle(const b2Vec2& center, float32 radius,
			const b2Color& color) {

	}

	/// Draw a solid circle.
	void DrawSolidCircle(const b2Vec2& center, float32 radius,
			const b2Vec2& axis, const b2Color& color) {
		float x1 = (center.x - myOffsetX) * myScaleX;
		float y1 = (center.y - myOffsetY) * myScaleY;

		//FIXME: only supports xscale==yscale
		//circle(myBuffer, x1, y1, radius*myScaleX, makecol(color.r*255, color.g*255, color.b*255));
	}

	/// Draw a line segment.
	void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) {
		float x1 = (p1.x - myOffsetX) * myScaleX;
		float y1 = (p1.y - myOffsetY) * myScaleY;
		float x2 = (p2.x - myOffsetX) * myScaleX;
		float y2 = (p2.y - myOffsetY) * myScaleY;

		sf::Vertex line[] = { sf::Vertex(sf::Vector2f(x1, y1)), sf::Vertex(
				sf::Vector2f(x2, y2)) };

		myBuffer->draw(line, 2, sf::Lines);
	}

	/// Draw a transform. Choose your own length scale.
	/// @param xf a transform.
	void DrawTransform(const b2Transform& xf) {

	}

	void Set(sf::RenderTarget& aTarget, float aScaleX, float aScaleY,
			float aOffsetX, float aOffsetY) {
		myBuffer = &aTarget;

		myScaleX = aScaleX;
		myScaleY = aScaleY;
		myOffsetX = aOffsetX;
		myOffsetY = aOffsetY;

	}
private:
	sf::RenderTarget* myBuffer;

	float myScaleX;
	float myScaleY;
	float myOffsetX;
	float myOffsetY;
};

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

	debugdraw* drawer = new debugdraw();
	myCollisionWorld.SetDebugDraw(drawer);
	drawer->SetFlags(0);

	myContactListener = new ContactListener(myCollisionWorld);
}

World::~World() {
	// TODO Auto-generated destructor stub
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

	debugdraw drawer;
	drawer.SetFlags(b2Draw::e_jointBit | b2Draw::e_shapeBit);
	myCollisionWorld.SetDebugDraw(&drawer);
	drawer.Set(aTarget, xscale, yscale, xtrans, ytrans);
	myCollisionWorld.DrawDebugData();
}
