// Leafwind -- sound effects, loops, ambience and generative music (docs/DESIGN.md section 6).
// Constructed disabled in headless modes: no OpenAL objects are created at all then.
#pragma once

#include "core/Events.h"
#include "core/Level.h"

#include <SFML/Audio.hpp>

#include <memory>
#include <vector>

class World;

enum class Sfx {
	Jump,
	LandSoft,
	LandHard,
	Step,
	StepWood,
	Banana0, // .. Banana7 (combo semitones)
	Fig = Banana0 + 8,
	RopeGrab,
	RopeRelease,
	ClimbTick,
	Splash,
	Drip,
	Hurt,
	Checkpoint,
	LevelComplete,
	Boing,
	Squish,
	CrumbleShake,
	CrumbleCrack,
	LogCreak,
	LogThud,
	BoulderImpact,
	Thunder,
	GustWhoosh,
	UiMove,
	UiConfirm,
	Count
};

class Audio {
public:
	explicit Audio(bool enabled);
	~Audio();
	bool enabled() const { return myEnabled; }

	void play(Sfx s, float volume = 1.f, float pitch = 1.f);
	void setVolume(float master);
	void setMusicEnabled(bool on) { musicOn = on; }

	// Build the ambience + music loops for a biome (cheap: a few hundred ms of synthesis at most).
	void setBiome(Biome b);
	// musicLevel: 1 in play, 0 during story cards (ambience only), fades smoothly.
	void update(const World* w, float dt, float musicLevel);
	void handleEvents(const EventList& ev);
	void stopLoops();

private:
	struct Loop {
		std::unique_ptr<sf::SoundBuffer> buf; // allocated lazily: no OpenAL use when disabled
		std::unique_ptr<sf::Sound> snd;
		float vol = 0.f;     // current (0..1)
		float target = 0.f;
	};
	void makeLoop(Loop& l, const std::vector<float>& samples);
	void driveLoop(Loop& l, float dt, float rate = 4.f);

	bool myEnabled;
	bool musicOn = true;
	float master = 0.8f;
	std::vector<sf::SoundBuffer> bufs;
	std::vector<std::unique_ptr<sf::Sound>> pool;
	size_t next = 0;
	Loop ambience, music, windLo, windHi, crate, boulder, updraft;
	Biome biome = Biome::Count;
	float stepPitchSeed = 0.f;
};
