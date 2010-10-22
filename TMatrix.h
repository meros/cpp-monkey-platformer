/*
 * TMatrix.h
 *
 *  Created on: 3 maj 2010
 *      Author: meros
 */

#ifndef TMATRIX_H_
#define TMATRIX_H_

#include "Point.h"

class TMatrix {
public:
				TMatrix();
	virtual 	~TMatrix();

	void 		Rotate(
					float 	aAngleRad);

	void 		Apply(
					Point&	aPoint);
private:
	float myMatrix[2][2];
};

#endif /* TMATRIX_H_ */
