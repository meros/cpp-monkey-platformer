/*
 * Sprite.cpp
 *
 *  Created on: 5 maj 2010
 *      Author: meros
 */

#include "Sprite.h"

#include <math.h>
#include <iostream>
using namespace std;

Sprite::Sprite() {
}

Sprite::~Sprite() {
	//	destroy_bitmap(myBMP);
}

bool Sprite::LoadTGA(const char* aTgaFile) {
	if (!aTgaFile)
		return false;

	if (!(myBMP.LoadFromFile(aTgaFile)))
		return false;

	mySprite.SetImage(myBMP);

	return true;
}

void 
Sprite::DrawGroundCenterRelative(
	sf::RenderTarget& aTarget,
	float aXCenter, 
	float aYGround, 
	float aHeight,
	float				aRotation) {



	float factor = (float) aHeight / myBMP.GetHeight();
	mySprite.SetScale(factor, factor);
	mySprite.SetPosition(aXCenter - mySprite.GetSize().x / 2.0,
			aYGround - aHeight);
	mySprite.SetRotation(-aRotation/(2*3.1415)*360);
	aTarget.Draw(mySprite);
}

void Sprite::DrawTiling(
		sf::RenderTarget& 	aTarget,
		float 				aXStart,
		float 				aYStart,
		float 				aXEnd,
		float 				aYEnd)
{

	float factor = (float) (aYEnd - aYStart) / myBMP.GetHeight();
	mySprite.SetScale(factor, factor);

	int numreps = floor(0.5 + (aXEnd - aXStart) / mySprite.GetSize().y);

	int reps = 0;
	for (float x = aXStart; true; x += mySprite.GetSize().y) {
		//FIXME: uhm.. ugly
		reps++;
		if (reps > numreps)
			break;

		mySprite.SetPosition(x, aYStart);
		aTarget.Draw(mySprite);
	}
}


void
Sprite::DrawStretchRotation(
	sf::RenderTarget&	aTarget,
	float  				aXCenter,
	float  				aYCenter,
	float				aNonRotHeight,
	float  				aAngleRad)
{
	float factor = (aNonRotHeight/myBMP.GetHeight());

	mySprite.SetScale(factor, factor);
	mySprite.SetCenter(myBMP.GetWidth()/2.0, myBMP.GetHeight()/2.0);
	mySprite.SetPosition(aXCenter, aYCenter);
	mySprite.SetRotation(-aAngleRad/(2*3.1415)*360);

	aTarget.Draw(mySprite);
}

void
Sprite::Mirror()
{
	mySprite.FlipX(true);
}
