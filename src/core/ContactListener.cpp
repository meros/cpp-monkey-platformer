#include "ContactListener.h"

#include "Entities.h"
#include "World.h"

void ContactListener::PreSolve(b2Contact* contact, const b2Manifold*) {
	b2Fixture* fa = contact->GetFixtureA();
	b2Fixture* fb = contact->GetFixtureB();
	Entity* ea = entityOf(fa->GetBody());
	Entity* eb = entityOf(fb->GetBody());
	if (!ea || !eb) return;

	b2Fixture* platform = nullptr;
	if (ea->kind == Kind::OneWay && eb->kind == Kind::Player) platform = fa;
	else if (eb->kind == Kind::OneWay && ea->kind == Kind::Player) platform = fb;
	if (!platform) return;

	const Player& p = myWorld.player;
	float top = platform->GetAABB(0).lowerBound.y + b2_polygonRadius;
	if (p.dropThrough > 0.f || p.bottom() > top + 0.04f || p.vel().y < -0.05f)
		contact->SetEnabled(false);
}
