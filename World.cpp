/*
 * World.cpp
 *
 *  Created on: 10 maj 2010
 *      Author: meros
 */

#include "World.h"

#include <fstream>

using namespace std;


class debugdraw
: public b2DebugDraw
{
public:
	/// Draw a closed polygon provided in CCW order.
	void 			DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
	{
		for (int i = 0; i < vertexCount; i++)
		{
			float x1 = (vertices[i].x-myOffsetX)*myScaleX;
			float y1 = (vertices[i].y-myOffsetY)*myScaleY;
			float x2 = (vertices[(i+1)%vertexCount].x-myOffsetX)*myScaleX;
			float y2 = (vertices[(i+1)%vertexCount].y-myOffsetY)*myScaleY;

			sf::Shape Line   = sf::Shape::Line(x1, y1, x2, y2, 1.0f, sf::Color::Black);
			//myBuffer->Draw(Line);
		}
	}

	/// Draw a solid closed polygon provided in CCW order.
	void 			DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
	{
				for (int i = 0; i < vertexCount; i++)
				{
					float x1 = (vertices[i].x-myOffsetX)*myScaleX;
					float y1 = (vertices[i].y-myOffsetY)*myScaleY;
					float x2 = (vertices[(i+1)%vertexCount].x-myOffsetX)*myScaleX;
					float y2 = (vertices[(i+1)%vertexCount].y-myOffsetY)*myScaleY;


					sf::Shape Line   = sf::Shape::Line(x1, y1, x2, y2, 1.0f, sf::Color::Black);
					myBuffer->Draw(Line);
				}
	}

	/// Draw a circle.
	void 			DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color)
	{

	}

	/// Draw a solid circle.
	void 			DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color)
	{
		float x1 = (center.x-myOffsetX)*myScaleX;
		float y1 = (center.y-myOffsetY)*myScaleY;

		//FIXME: only supports xscale==yscale
		//circle(myBuffer, x1, y1, radius*myScaleX, makecol(color.r*255, color.g*255, color.b*255));
	}

	/// Draw a line segment.
	void 			DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
	{
		float x1 = (p1.x-myOffsetX)*myScaleX;
		float y1 = (p1.y-myOffsetY)*myScaleY;
		float x2 = (p2.x-myOffsetX)*myScaleX;
		float y2 = (p2.y-myOffsetY)*myScaleY;

		sf::Shape Line   = sf::Shape::Line(x1,  y1, x2, y2, 1.0f, sf::Color(color.r*255, color.g*255, color.b*255));
		myBuffer->Draw(Line);
	}

	/// Draw a transform. Choose your own length scale.
	/// @param xf a transform.
	void DrawTransform(const b2Transform& xf)
	{
		
	}

	void			Set(
							sf::RenderTarget&	aTarget,
							float 		aScaleX,
							float 		aScaleY,
							float 		aOffsetX,
							float		aOffsetY)
	{
		myBuffer = &aTarget;

		myScaleX = aScaleX;
		myScaleY = aScaleY;
		myOffsetX = aOffsetX;
		myOffsetY = aOffsetY;

	}
private:
	 sf::RenderTarget* myBuffer;

	float 		myScaleX;
	float 		myScaleY;
	float 		myOffsetX;
	float		myOffsetY;
};

#define BLOCK_SIZE 0.20

World::World()
: myCollisionWorld(b2Vec2(0.0f, 20.0f), true)
, myFrameCounter(0)
{
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
		while (*block)
		{
			if (*block == 'P')
			{
				px = x;
				py = y;
			}

			if (*block == 'X' && startpos == -1)
			{
				startpos = x;
			}
			else if (*block != 'X' && startpos != -1)
			{
				myBlocks.push_back(new Block(myCollisionWorld, startpos*BLOCK_SIZE, y*BLOCK_SIZE, BLOCK_SIZE*(x-startpos), BLOCK_SIZE));
				startpos = -1;
			}

			//Create rope
			if (*block == 'R')
			{
				myRopes.push_back(new Rope(myCollisionWorld, (x+0.5f)*BLOCK_SIZE, y*BLOCK_SIZE));
			}

			//Create bridge
			if (*block == 'B')
			{
				//myRopes.push_back(new Rope(myCollisionWorld, (x+0.5f)*BLOCK_SIZE, y*BLOCK_SIZE));

				b2Body* last = NULL;

				for (int i = 0; i < 10; i++)
				{
					float BRIDGE_LENGTH = 0.1;
					float BRIDGE_WIDTH = 0.1;

					b2BodyDef bridgeDef;
					bridgeDef.position = b2Vec2(x*BLOCK_SIZE+i*BRIDGE_WIDTH*2, y*BLOCK_SIZE);
					//dropboxDef.angle = 0.2;

					if (i != 0 && i != 9)
					{
						bridgeDef.type = b2_dynamicBody;
					}

					b2PolygonShape bridgeShape;

					bridgeShape.SetAsBox(BRIDGE_WIDTH/2, BRIDGE_LENGTH/2);

					b2Body* bridgeBody = myCollisionWorld.CreateBody(&bridgeDef);
					bridgeBody->CreateFixture(&bridgeShape, 0.05f);

					if (i != 0)
					{
						b2DistanceJointDef jointDef;
						jointDef.Initialize(last, bridgeBody, last->GetPosition()+b2Vec2(BRIDGE_WIDTH/2, 0), bridgeBody->GetPosition()-b2Vec2(BRIDGE_WIDTH/2, 0));
						jointDef.length = BRIDGE_WIDTH*1.1;

						myCollisionWorld.CreateJoint(&jointDef);
					}

					//myParts.push_back(body2);

					last = bridgeBody;
				}
			}


			block++;
			x++;
		}

		if (startpos != -1)
		{
			myBlocks.push_back(new Block(myCollisionWorld, startpos*BLOCK_SIZE, y*BLOCK_SIZE, BLOCK_SIZE*(x-startpos), BLOCK_SIZE));
		}
		
		startpos = -1;
		 y++;
	 }

	 myPlayer = new Player(myCollisionWorld, (px+0.5)*BLOCK_SIZE, (py+0.5)*BLOCK_SIZE);

	 debugdraw* drawer = new debugdraw();
	 myCollisionWorld.SetDebugDraw(drawer);
	 //drawer->SetFlags(0);

         myContactListener = new ContactListener(myCollisionWorld);
}

World::~World() {
	// TODO Auto-generated destructor stub
}

void
World::Update(
		const sf::Input& aInput)
{
	myPlayer->Update(aInput);

	vector<Rope*>::iterator it;
	for (it = myRopes.begin(); it != myRopes.end(); it++)
		(*it)->Update();

	myCollisionWorld.Step(1.0f/50.0f, 50, 50);
}

void
World::Draw(
		sf::RenderTarget&	aTarget)
{
	myFrameCounter++;

	float xscale = 120;
	float yscale = xscale;

	float xtrans = myPlayer->GetPosition().myX-(400/xscale);
	float ytrans = myPlayer->GetPosition().myY-(300/yscale);

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


	debugdraw drawer;
	drawer.SetFlags(b2DebugDraw::e_jointBit|b2DebugDraw::e_shapeBit);

	myCollisionWorld.SetDebugDraw(&drawer);
	drawer.Set(aTarget, xscale, yscale, xtrans, ytrans);
        //myCollisionWorld.DrawDebugData();
}
