/*
 * TMatrix.cpp
 *
 *  Created on: 3 maj 2010
 *      Author: meros
 */

#include "TMatrix.h"

#include <math.h>

TMatrix::TMatrix() {
	myMatrix[0][0] = 1.0f;
	myMatrix[1][1] = 1.0f;
	myMatrix[0][1] = 0.0f;
	myMatrix[1][0] = 0.0f;
}

TMatrix::~TMatrix() {
	// TODO Auto-generated destructor stub
}

void
TMatrix::Rotate(
		float aAngleRad)
{
	float rotmatrix[2][2];
	rotmatrix[0][0] = cos(aAngleRad);
	rotmatrix[1][1] = rotmatrix[0][0];

	rotmatrix[0][1] = sin(aAngleRad);
	rotmatrix[1][0] = -rotmatrix[0][1];

	float result[2][2];

	for (int x = 0; x < 2; x++)
		for (int y = 0; y < 2; y++)
			result[x][y] = myMatrix[0][y]*rotmatrix[x][0]+myMatrix[1][y]*rotmatrix[x][1];

	for (int x = 0; x < 2; x++)
		for (int y = 0; y < 2; y++)
			myMatrix[x][y] = result[x][y];
}

void
TMatrix::Apply(
		Point& aPoint)
{
	Point res;
	res.myX = aPoint.myX*myMatrix[0][0]+aPoint.myY*myMatrix[1][0];
	res.myY = aPoint.myX*myMatrix[0][1]+aPoint.myY*myMatrix[1][1];

	aPoint = res;
}

