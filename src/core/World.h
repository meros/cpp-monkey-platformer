// Leafwind -- the simulation: Box2D world, level geometry, objects and the player.
// No graphics here; the renderer only reads this state (docs/DESIGN.md 8.1).
#pragma once

#include "Entities.h"
#include "Events.h"
#include "Input.h"
#include "Level.h"
#include "Player.h"

#include <box2d/box2d.h>

#include <memory>
#include <vector>

class ContactListener;

class World {
public:
	explicit World(const LevelData& level);
	~World();
	World(const World&) = delete;
	World& operator=(const World&) = delete;

	void step(const Input& in);

	// Death / respawn
	void killPlayer();
	void respawn();          // rebuild all dynamic objects, player at checkpoint

	// Queries used by the player and objects
	uint8_t waterAtM(float x, float y) const;
	uint8_t windAtM(float x, float y) const;
	float waterSurfaceAt(float x, float y) const; // top of the water column containing (x,y)
	bool solidTileAtM(float x, float y) const;
	bool windActive() const { return !level.gust || gustOn; }
	int windDirAt(float x, float y) const; // -1, 0, 1 (horizontal wind only, respects gusts)

	void emit(Ev t, float x, float y, float v = 0.f) { events.push_back({t, x, y, v}); }

	// debug / tests
	void placePlayer(float x, float y); // centre, metres
	b2Vec2 spawnPoint() const;

	const LevelData& level;
	std::unique_ptr<b2World> b2;
	std::unique_ptr<ContactListener> contacts;
	Player player;
	Entity terrainEntity{Kind::Terrain};
	Entity oneWayEntity{Kind::OneWay};
	b2Body* ground = nullptr;   // static terrain body
	b2Body* anchor = nullptr;   // fixture-less static body used as the ground side of joints

	std::vector<std::unique_ptr<Rope>> ropes;
	std::vector<std::unique_ptr<Bridge>> bridges;
	std::vector<std::unique_ptr<Crate>> crates;
	std::vector<std::unique_ptr<Boulder>> boulders;
	std::vector<std::unique_ptr<FloatLog>> floatLogs;
	std::vector<std::unique_ptr<FallLog>> fallLogs;
	std::vector<std::unique_ptr<Beetle>> beetles;
	std::vector<std::unique_ptr<Mover>> movers;
	std::vector<std::unique_ptr<Seesaw>> seesaws;
	std::vector<std::unique_ptr<Lift>> lifts;
	std::vector<std::unique_ptr<Crumble>> crumbles;
	std::vector<std::unique_ptr<Mushroom>> mushrooms;

	std::vector<Collectible> items;
	std::vector<Checkpoint> checkpoints;
	std::vector<Sign> signs;
	std::vector<Thorn> thorns;
	int exitX = 0, exitY = 0;
	int startX = 0, startY = 0;
	int activeCheckpoint = -1;

	EventList events;
	float time = 0.f;         // sim time since level start (never reset by deaths)
	float levelTimer = 0.f;   // play time (stops at completion)
	int deaths = 0;
	int bananas = 0;
	int bananasTotal = 0;
	bool fig = false;
	bool complete = false;
	int bananaCombo = 0;
	float comboTimer = 0.f;

	// gusts (3.13)
	bool gustOn = true;
	float gustTimer = 0.f;
	bool gustWarned = false;
	float windEnvelope = 1.f; // 0..1 visual/audio strength

	// feedback for audio loops
	float cratePush = 0.f;
	float boulderRoll = 0.f;
	int frame = 0;

private:
	void build();
	void buildTerrain();
	void buildObjects();
	void updateObjects(float dt);
	void postObjects(float dt);
	void applyWaterForces(b2Body* b, float floatFrac);
	void updateGust(float dt);
	void checkItems();
};
