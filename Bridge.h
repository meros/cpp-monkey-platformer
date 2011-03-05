#ifndef BRIDGE_H
#define BRIDGE_H

#include "ContactListener.h"
#include "Sprite.h"

#include <SFML/Graphics.hpp>
#include <Box2D/Box2D.h>
#include <vector>

#include "IDrawable.h"

class Bridge        
    : public IDrawable
{
public:
    Bridge(
            b2World&            aWorld,
            float		aX,
            float		aY);

    void Draw(sf::RenderTarget &aTarget, float aXScale, float aYScale, float aXTranslate, float aYTranslate);

private:
    b2World& myWorld;
    std::vector<b2Body*>	myParts;
};

#endif // BRIDGE_H
