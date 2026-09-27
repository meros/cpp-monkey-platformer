// Leafwind -- Box2D contact callbacks: one-way branches (docs/DESIGN.md 3.1).
#pragma once

#include <box2d/box2d.h>

class World;

class ContactListener : public b2ContactListener {
public:
	explicit ContactListener(World& w) : myWorld(w) {}
	void PreSolve(b2Contact* contact, const b2Manifold* oldManifold) override;

private:
	World& myWorld;
};
