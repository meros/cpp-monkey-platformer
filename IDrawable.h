/*
 * IDrawable.h
 *
 *  Created on: 3 maj 2010
 *      Author: meros
 */

#ifndef IDRAWABLE_H_
#define IDRAWABLE_H_

#include "TMatrix.h"

#include <SFML/Graphics.hpp>

class IDrawable {
public:
	virtual void Draw(sf::RenderTarget& aTarget, float aXScale, float aYScale,
			float aXTranslate, float aYTranslate) = 0;
};

#endif /* IDRAWABLE_H_ */
