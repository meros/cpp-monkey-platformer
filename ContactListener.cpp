/*
 * ContactListener.cpp
 *
 *  Created on: 8 maj 2010
 *      Author: meros
 */

#include "ContactListener.h"


ContactListener::ContactListener(b2World& aWorld) {
	aWorld.SetContactListener(this);
}

ContactListener::~ContactListener() {
}

static UserData* GetUserDataPtr(b2Fixture* fixture) {
	return reinterpret_cast<UserData*>(fixture->GetUserData().pointer);
}

void ContactListener::BeginContact(b2Contact* contact) {
	UserData* callbackA = GetUserDataPtr(contact->GetFixtureA());
	UserData* callbackB = GetUserDataPtr(contact->GetFixtureB());

	if (callbackA) {
		callbackA->BeginContact(contact);
	}

	if (callbackB) {
		callbackB->BeginContact(contact);
	}
}

void ContactListener::EndContact(b2Contact* contact) {
	UserData* callbackA = GetUserDataPtr(contact->GetFixtureA());
	UserData* callbackB = GetUserDataPtr(contact->GetFixtureB());

	if (callbackA) {
		callbackA->EndContact(contact);
	}

	if (callbackB) {
		callbackB->EndContact(contact);
	}
}

void ContactListener::PreSolve(b2Contact* contact,
		const b2Manifold* oldManifold) {
	UserData* callbackA = GetUserDataPtr(contact->GetFixtureA());
	UserData* callbackB = GetUserDataPtr(contact->GetFixtureB());

	if (callbackA) {
		callbackA->PreSolve(contact, oldManifold);
	}

	if (callbackB) {
		callbackB->PreSolve(contact, oldManifold);
	}
}

void ContactListener::PostSolve(b2Contact* contact,
		const b2ContactImpulse* impulse) {
	UserData* callbackA = GetUserDataPtr(contact->GetFixtureA());
	UserData* callbackB = GetUserDataPtr(contact->GetFixtureB());

	if (callbackA) {
		callbackA->PostSolve(contact, impulse);
	}

	if (callbackB) {
		callbackB->PostSolve(contact, impulse);
	}
}
