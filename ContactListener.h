/*
 * ContactListener.h
 *
 *  Created on: 8 maj 2010
 *      Author: meros
 */

#ifndef CONTACTLISTENER_H_
#define CONTACTLISTENER_H_

#include <Box2D/Box2D.h>

class Rope;

class UserData {
public:
	UserData() :
			myType(Unknown) {
	}

	virtual void BeginContact(b2Contact* contact) {
	}

	virtual void EndContact(b2Contact* contact) {
	}

	virtual void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) {
	}

	virtual void PostSolve(b2Contact* contact,
			const b2ContactImpulse* impulse) {
	}

	virtual void TouchedRope(Rope* aRope, b2Body* aBpdy) {
	}

	enum Type {
		Unknown, Player,
	} myType;
};

class ContactListener: public b2ContactListener {
public:
	ContactListener(b2World& aWorld);
	virtual ~ContactListener();

	void BeginContact(b2Contact* contact);

	void EndContact(b2Contact* contact);

	void PreSolve(b2Contact* contact, const b2Manifold* oldManifold);
	void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse);
};

#endif /* CONTACTLISTENER_H_ */
