/*
 * DummyDrawable.cpp
 *
 *  Created on: 3 maj 2010
 *      Author: meros
 */

#include "DummyDrawable.h"

DummyDrawable::DummyDrawable() {
	// TODO Auto-generated constructor stub

}

DummyDrawable::~DummyDrawable() {
	// TODO Auto-generated destructor stub
}

void DummyDrawable::Draw(sf::Window& aTarget, TMatrix* aWorld2Bitmap) {
	Point p;

	for (int xdiff = -10; xdiff <= 10; xdiff++)
		for (int ydiff = -10; ydiff <= 10; ydiff++) {
			p.myX = xdiff;
			p.myY = ydiff;

			myRotMatrix.Apply(p);

			//FIXME draw something
		}
}

void DummyDrawable::SetPosition(float aX, float aY) {
	myX = aX;
	myY = aY;
}

void DummyDrawable::SetRotation(float aRotation) {
	myRotMatrix.Rotate(aRotation);
}
