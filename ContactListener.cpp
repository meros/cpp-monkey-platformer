/*
 * ContactListener.cpp
 *
 *  Created on: 8 maj 2010
 *      Author: meros
 */

#include "ContactListener.h"

#include <iostream>

using namespace std;

ContactListener::ContactListener(
		b2World&	aWorld)
{
	aWorld.SetContactListener(this);
}

ContactListener::~ContactListener() {
	// TODO Auto-generated destructor stub
}


void
ContactListener::BeginContact(
		b2Contact*	 contact)
{
	UserData* callbackA = (UserData*)(contact->GetFixtureA()->GetUserData());
	UserData* callbackB = (UserData*)(contact->GetFixtureB()->GetUserData());

	if (callbackA)
	{
		callbackA->BeginContact(contact);
	}

	if (callbackB)
	{
		callbackB->BeginContact(contact);
	}
}


void
ContactListener::EndContact(
		b2Contact* 	contact)
{
	UserData* callbackA = (UserData*)(contact->GetFixtureA()->GetUserData());
	UserData* callbackB = (UserData*)(contact->GetFixtureB()->GetUserData());

	if (callbackA)
	{
		callbackA->EndContact(contact);
	}

	if (callbackB)
	{
		callbackB->EndContact(contact);
	}
}

void
ContactListener::PreSolve(
	b2Contact* 			contact,
	const b2Manifold* 	oldManifold)
{
	UserData* callbackA = (UserData*)(contact->GetFixtureA()->GetUserData());
	UserData* callbackB = (UserData*)(contact->GetFixtureB()->GetUserData());

	if (callbackA)
	{
		callbackA->PreSolve(contact, oldManifold);
	}

	if (callbackB)
	{
		callbackB->PreSolve(contact, oldManifold);
	}
}

void
ContactListener::PostSolve(
		b2Contact* 				contact,
		const b2ContactImpulse*	impulse)
{
	UserData* callbackA = (UserData*)(contact->GetFixtureA()->GetUserData());
	UserData* callbackB = (UserData*)(contact->GetFixtureB()->GetUserData());

	if (callbackA)
	{
		callbackA->PostSolve(contact, impulse);
	}

	if (callbackB)
	{
		callbackB->PostSolve(contact, impulse);
	}
}
