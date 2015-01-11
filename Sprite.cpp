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

	if (!(myBMP.loadFromFile(aTgaFile)))
		return false;

	mySprite.setTexture(myBMP);

	return true;
}

void Sprite::DrawGroundCenterRelative(sf::RenderTarget& aTarget, float aXCenter,
		float aYGround, float aHeight, float aRotation) {

	float factor = (float) aHeight / myBMP.getSize().y;
	mySprite.setScale(factor, factor);
	mySprite.setPosition(aXCenter - mySprite.getTextureRect().width / 2.0,
			aYGround - aHeight);
	mySprite.setRotation(-aRotation / (2 * 3.1415) * 360);
	aTarget.draw(mySprite);
}

void Sprite::DrawTiling(sf::RenderTarget& aTarget, float aXStart, float aYStart,
		float aXEnd, float aYEnd) {

	float factor = (float) (aYEnd - aYStart) / myBMP.getSize().y;
	mySprite.setScale(factor, factor);

	int numreps = floor(
			0.5 + (aXEnd - aXStart) / mySprite.getTextureRect().width);

	int reps = 0;
	for (float x = aXStart; true; x += mySprite.getTextureRect().width) {
		//FIXME: uhm.. ugly
		reps++;
		if (reps > numreps)
			break;

		mySprite.setPosition(x, aYStart);
		aTarget.draw(mySprite);
	}
}

void Sprite::DrawStretchRotation(sf::RenderTarget& aTarget, float aXCenter,
		float aYCenter, float aNonRotHeight, float aAngleRad) {
	float factor = (aNonRotHeight / myBMP.getSize().y);

	mySprite.setScale(factor, factor);
	mySprite.setOrigin(myBMP.getSize().x / 2.0, myBMP.getSize().y / 2.0);
	mySprite.setPosition(aXCenter, aYCenter);
	mySprite.setRotation(aAngleRad / (2 * 3.1415) * 360);

	aTarget.draw(mySprite);
}

void Sprite::Mirror() {
	sf::IntRect rect = mySprite.getTextureRect();
	rect.width = -rect.width;
	mySprite.setTextureRect(rect);
}
