#include "Bridge.h"

Bridge::Bridge(
        b2World&	aWorld,
        float		aX,
        float		aY)
    : myWorld(aWorld)
{
    //myRopes.push_back(new Rope(myCollisionWorld, (x+0.5f)*BLOCK_SIZE, y*BLOCK_SIZE));

    b2Body* last = NULL;

    for (int i = 0; i < 10; i++)
    {
            float BRIDGE_LENGTH = 0.1;
            float BRIDGE_WIDTH = 0.1;

            b2BodyDef bridgeDef;
            bridgeDef.position = b2Vec2(aX+i*BRIDGE_WIDTH*2, aY
                                        );
            //dropboxDef.angle = 0.2;

            if (i != 0 && i != 9)
            {
                    bridgeDef.type = b2_dynamicBody;
            }

            b2PolygonShape bridgeShape;

            bridgeShape.SetAsBox(BRIDGE_WIDTH/2, BRIDGE_LENGTH/2);

            b2Body* bridgeBody = myWorld.CreateBody(&bridgeDef);
            bridgeBody->CreateFixture(&bridgeShape, 0.05f);

            if (i != 0)
            {
                    b2DistanceJointDef jointDef;
                    jointDef.Initialize(last, bridgeBody, last->GetPosition()+b2Vec2(BRIDGE_WIDTH/2, 0), bridgeBody->GetPosition()-b2Vec2(BRIDGE_WIDTH/2, 0));
                    jointDef.length = BRIDGE_WIDTH*1.1;

                    myWorld.CreateJoint(&jointDef);
            }

            myParts.push_back(bridgeBody);

            last = bridgeBody;
    }
}

void
Bridge::Draw(
        sf::RenderTarget&   aTarget,
        float               aXScale,
        float               aYScale,
        float               aXTranslate,
        float               aYTranslate)
{

}
