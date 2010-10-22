/*
 * Sprite.h
 *
 *  Created on: 5 maj 2010
 *      Author: meros
 */

#ifndef SPRITEDRAWABLE_H_
#define SPRITEDRAWABLE_H_

#include <SFML/Graphics.hpp>

class Sprite{
public:
				Sprite();
	virtual 	~Sprite();

	bool 		LoadTGA(
					const char* aTgaFile);

	void 		Mirror();

	void 		DrawGroundCenterRelative(
					sf::RenderTarget&	aTarget,
					float 				aXCenter,
					float 				aYGround,
					float 				aHeight,
					float				aRotation);

	void 		DrawTiling(
					sf::RenderTarget&	aTarget,
					float  				aXStart,
					float  				aYStart,
					float  				aXEnd,
					float  				aYEnd);

	void 		DrawStretchRotation(
						sf::RenderTarget&	aTarget,
						float  				aXCenter,
						float  				aYCenter,
						float				aNonRotHeight,
						float  				aAngleRad);

private:
	sf::Image 	myBMP;
	sf::Sprite  mySprite;
};

#endif /* Sprite_H_ */
