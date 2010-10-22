/*
 * DummyDrawable.h
 *
 *  Created on: 3 maj 2010
 *      Author: meros
 */

#ifndef DUMMYDRAWABLE_H_
#define DUMMYDRAWABLE_H_

#include "IDrawable.h"
#include "IPositionable.h"

class DummyDrawable
	: public IDrawable
	, public IPositionable
{
public:
	DummyDrawable();
	virtual ~DummyDrawable();


	void				Draw(
							sf::Window&	aTarget,
							TMatrix* 	aWorld2Bitmap);

	void 				SetPosition(
							float 		aX,
							float 		aY);

	void 				SetRotation(
							float 		aAngleRad);
private:
	float 	myX;
	float 	myY;

	TMatrix	myRotMatrix;
};

#endif /* DUMMYDRAWABLE_H_ */
