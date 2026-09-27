// Leafwind -- one-shot gameplay events emitted by the simulation and consumed by the
// renderer (particles, screen shake) and the audio system.
#pragma once

#include <vector>

enum class Ev {
	Jump,          // value: 1 normal, 1.1 from vine
	LandSoft,
	LandHard,
	Step,          // value: 1 = wood
	Banana,        // value: combo index 0..7
	Fig,
	RopeGrab,
	RopeRelease,
	ClimbTick,
	Splash,
	Drip,
	Death,
	Respawn,
	Checkpoint,
	ExitEnter,
	LevelComplete,
	Bounce,        // value: 1 normal, 1.25 big
	Squish,
	CrumbleShake,
	CrumbleFall,
	CrumbleReturn,
	LogCreak,
	LogThud,
	BoulderImpact, // value: strength
	GustWarn,
	GustStart,
	GustEnd,
	Swim,
};

struct GameEvent {
	Ev type;
	float x = 0.f, y = 0.f; // metres
	float value = 0.f;
};

using EventList = std::vector<GameEvent>;
