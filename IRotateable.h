/*
 * IRotateable.h
 *
 *  Created on: 3 maj 2010
 *      Author: meros
 */

#ifndef IROTATEABLE_H_
#define IROTATEABLE_H_

class IRotateable {
public:
	virtual void 	SetRotation(
						float aAngleRad) = 0;
};

#endif /* IROTATEABLE_H_ */
