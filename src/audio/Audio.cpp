#include "Audio.h"

#include "Synth.h"
#include "core/World.h"

#include <algorithm>
#include <cmath>

using namespace synth;

namespace {

Buf seamless(Buf b, float xfade) {
	int x = std::min(static_cast<int>(b.size()) / 3, samples(xfade));
	int n = static_cast<int>(b.size()) - x;
	Buf out(b.begin(), b.begin() + n);
	for (int i = 0; i < x; ++i) {
		float t = static_cast<float>(i) / x;
		out[i] = out[i] * t + b[n + i] * (1.f - t);
	}
	return out;
}

Buf sineNote(float f, float dur, float a, float d, bool expDecay = true) {
	Buf b = tone(Wave::Sine, f, f, dur);
	envAD(b, a, d, expDecay);
	return b;
}

Buf marimba(float f, float dur) {
	Buf b = tone(Wave::Tri, f, f, dur);
	envAD(b, 0.003f, 0.4f, true);
	Buf h = tone(Wave::Sine, f * 4.f, f * 4.f, 0.12f);
	envAD(h, 0.001f, 0.08f, true);
	mix(b, h, 0.25f);
	lowpass(b, 3500.f);
	return b;
}

Buf hardLand(float g) {
	Buf n = noise(0.04f, 11);
	lowpass(n, 600.f);
	envAD(n, 0.001f, 0.04f);
	gain(n, 0.6f);
	Buf th = tone(Wave::Sine, 60.f, 50.f, 0.08f);
	envAD(th, 0.002f, 0.078f);
	mix(n, th, 1.f);
	gain(n, g);
	return n;
}

std::vector<Buf> buildSfx() {
	std::vector<Buf> s(static_cast<size_t>(Sfx::Count));
	auto set = [&](Sfx k, Buf b) { s[static_cast<size_t>(k)] = std::move(b); };
	{ // jump
		Buf b = tone(Wave::Square, 220.f, 440.f, 0.09f, false, 0.4f);
		envAD(b, 0.002f, 0.08f);
		lowpass(b, 3000.f);
		gain(b, 0.35f);
		set(Sfx::Jump, b);
	}
	{ // land soft
		Buf b = noise(0.04f, 3);
		lowpass(b, 600.f);
		envAD(b, 0.001f, 0.04f);
		gain(b, 0.4f * 1.6f);
		set(Sfx::LandSoft, b);
	}
	set(Sfx::LandHard, hardLand(0.8f));
	{ // step
		Buf b = noise(0.015f, 5);
		lowpass(b, 1200.f);
		envAD(b, 0.001f, 0.014f);
		gain(b, 0.15f * 2.f);
		set(Sfx::Step, b);
		Buf w = tone(Wave::Tri, 200.f, 190.f, 0.03f);
		envAD(w, 0.001f, 0.028f, true);
		gain(w, 0.25f);
		set(Sfx::StepWood, w);
	}
	for (int k = 0; k < 8; ++k) { // banana combo
		float m = std::pow(2.f, k / 12.f);
		Buf a = sineNote(880.f * m, 0.06f, 0.001f, 0.06f, false);
		Buf b = sineNote(1320.f * m, 0.1f, 0.001f, 0.1f, true);
		Buf out = a;
		mix(out, b, 1.f, samples(0.06f));
		gain(out, 0.3f);
		s[static_cast<size_t>(Sfx::Banana0) + k] = out;
	}
	{ // golden fig
		Buf out(samples(0.8f), 0.f);
		const float f[4] = {523.f, 659.f, 784.f, 1047.f};
		for (int i = 0; i < 4; ++i) {
			Buf n = tone(Wave::Tri, f[i], f[i], 0.15f);
			envAD(n, 0.002f, 0.148f, true);
			mix(out, n, 0.4f, samples(0.09f * i));
		}
		Buf sh = sineNote(2093.f, 0.4f, 0.05f, 0.35f);
		mix(out, sh, 0.2f, samples(0.2f));
		set(Sfx::Fig, out);
	}
	{ // rope grab / release
		Buf slap = noise(0.025f, 7);
		bandpass(slap, 800.f, 2000.f);
		envAD(slap, 0.001f, 0.024f);
		gain(slap, 2.f);
		Buf th = tone(Wave::Tri, 160.f, 150.f, 0.06f);
		envAD(th, 0.002f, 0.058f);
		mix(slap, th, 0.5f);
		gain(slap, 0.5f);
		set(Sfx::RopeGrab, slap);
		Buf rel = reversed(slap);
		gain(rel, 0.5f);
		set(Sfx::RopeRelease, rel);
	}
	{ // climb tick
		Buf b = tone(Wave::Tri, 300.f, 300.f, 0.02f);
		envAD(b, 0.001f, 0.019f);
		gain(b, 0.2f);
		set(Sfx::ClimbTick, b);
	}
	{ // splash
		Buf b = noise(0.25f, 9);
		lowpassSweep(b, 4000.f, 400.f);
		envAD(b, 0.005f, 0.245f);
		gain(b, 0.9f);
		Rng r(4);
		for (int i = 0; i < 3; ++i) {
			float f = r(600.f, 1200.f);
			Buf blip = sineNote(f, 0.04f, 0.002f, 0.038f);
			mix(b, blip, 0.25f, samples(r(0.f, 0.1f)));
		}
		set(Sfx::Splash, b);
		Buf d = sineNote(1400.f, 0.05f, 0.001f, 0.045f);
		gain(d, 0.12f);
		set(Sfx::Drip, d);
	}
	{ // hurt
		Buf b = tone(Wave::Saw, 440.f, 110.f, 0.35f);
		for (size_t i = 0; i < b.size(); ++i) b[i] *= (std::fmod(i * 30.f / SR, 1.f) < 0.5f) ? 1.f : 0.45f;
		envAD(b, 0.002f, 0.35f);
		lowpass(b, 2500.f);
		gain(b, 0.35f);
		const float f[3] = {392.f, 330.f, 262.f};
		for (int i = 0; i < 3; ++i) {
			Buf n = tone(Wave::Tri, f[i], f[i], 0.14f);
			envAD(n, 0.004f, 0.13f, true);
			mix(b, n, 0.3f, samples(0.35f + 0.12f * i));
		}
		set(Sfx::Hurt, b);
	}
	{ // checkpoint
		Buf b = tone(Wave::Sine, 523.f, 523.f, 0.5f);
		mix(b, tone(Wave::Sine, 659.f, 659.f, 0.5f));
		mix(b, tone(Wave::Sine, 784.f, 784.f, 0.5f));
		tremolo(b, 6.f, 0.4f);
		envAD(b, 0.02f, 0.48f);
		gain(b, 0.14f);
		Buf tick = sineNote(2000.f, 0.02f, 0.001f, 0.019f, false);
		mix(b, tick, 0.2f);
		set(Sfx::Checkpoint, b);
	}
	{ // level complete
		Buf out(samples(1.4f), 0.f);
		const float f[8] = {523, 659, 784, 1047, 784, 659, 784, 1047};
		for (int i = 0; i < 8; ++i) {
			Buf n = tone(Wave::Tri, f[i], f[i], i == 7 ? 0.5f : 0.16f);
			envAD(n, 0.003f, i == 7 ? 0.49f : 0.15f, true);
			mix(out, n, 0.5f, samples(0.11f * i));
		}
		Buf pad = tone(Wave::Sine, 261.6f, 261.6f, 1.2f);
		mix(pad, tone(Wave::Sine, 392.f, 392.f, 1.2f));
		envADSR(pad, 0.05f, 0.2f, 0.7f, 0.4f);
		mix(out, pad, 0.15f);
		gain(out, 0.5f * 1.3f);
		set(Sfx::LevelComplete, out);
	}
	{ // mushroom boing
		Buf a = tone(Wave::Sine, 150.f, 600.f, 0.12f, true);
		Buf b = tone(Wave::Sine, 600.f, 300.f, 0.18f, true);
		Buf out = concat(a, b);
		vibratoPitch(out, 12.f, 0.04f);
		envAD(out, 0.003f, 0.29f);
		gain(out, 0.4f);
		set(Sfx::Boing, out);
	}
	{ // beetle squish
		Buf n = noise(0.06f, 21);
		bandpass(n, 300.f, 900.f);
		gain(n, 2.5f);
		Buf pop = tone(Wave::Sine, 90.f, 70.f, 0.08f);
		mix(n, pop, 0.6f);
		envAD(n, 0.001f, 0.08f);
		gain(n, 0.6f);
		set(Sfx::Squish, n);
	}
	{ // crumble
		Buf sh = noise(0.35f, 23);
		lowpass(sh, 300.f);
		tremolo(sh, 20.f, 0.8f);
		envADSR(sh, 0.02f, 0.1f, 0.8f, 0.08f);
		gain(sh, 0.3f * 3.f);
		set(Sfx::CrumbleShake, sh);
		Buf cr = noise(0.15f, 29);
		lowpass(cr, 1500.f);
		envAD(cr, 0.001f, 0.15f, true);
		Buf click = noise(0.002f, 31);
		mix(cr, click, 1.f);
		gain(cr, 0.6f);
		set(Sfx::CrumbleCrack, cr);
	}
	{ // log creak & thud
		Buf b = tone(Wave::Saw, 180.f, 180.f, 0.3f);
		vibratoPitch(b, 5.f, 0.08f);
		lowpass(b, 1000.f);
		envADSR(b, 0.03f, 0.1f, 0.8f, 0.1f);
		gain(b, 0.3f);
		set(Sfx::LogCreak, b);
		set(Sfx::LogThud, hardLand(1.0f));
	}
	{ // boulder impact
		Buf b = tone(Wave::Sine, 55.f, 45.f, 0.15f);
		envAD(b, 0.002f, 0.148f);
		Buf n = noise(0.1f, 37);
		lowpass(n, 800.f);
		envAD(n, 0.001f, 0.1f, true);
		mix(b, n, 0.6f);
		gain(b, 0.8f);
		set(Sfx::BoulderImpact, b);
	}
	{ // thunder
		Buf b = brownNoise(2.2f, 41);
		lowpass(b, 180.f);
		envAD(b, 0.08f, 2.1f, true);
		normalize(b, 0.6f);
		set(Sfx::Thunder, b);
	}
	{ // gust warning whoosh (rising)
		Buf b = noise(0.6f, 43);
		lowpassSweep(b, 200.f, 1500.f);
		envADSR(b, 0.5f, 0.05f, 1.f, 0.05f);
		gain(b, 0.5f);
		set(Sfx::GustWhoosh, b);
	}
	{ // UI
		Buf m = sineNote(660.f, 0.03f, 0.001f, 0.029f, false);
		gain(m, 0.25f);
		set(Sfx::UiMove, m);
		Buf c = sineNote(880.f, 0.08f, 0.001f, 0.079f);
		mix(c, sineNote(1320.f, 0.08f, 0.001f, 0.079f));
		gain(c, 0.2f);
		set(Sfx::UiConfirm, c);
	}
	return s;
}

// ---- ambience (8 s loops) ----
Buf buildAmbience(Biome b) {
	const float L = 8.f;
	Rng r(static_cast<unsigned>(b) * 97u + 3u);
	Buf out = pinkNoise(L + 0.5f, 51 + static_cast<unsigned>(b));
	lowpass(out, 2500.f);
	normalize(out, 0.06f);
	out = seamless(out, 0.5f);
	auto sprinkle = [&](float minGap, float maxGap, auto make) {
		float t = r(0.f, minGap);
		while (t < L) {
			Buf e = make();
			mix(out, e, 1.f, samples(t), true);
			t += r(minGap, maxGap);
		}
	};
	switch (b) {
	case Biome::Canopy:
	case Biome::Grove:
	case Biome::Gully:
	case Biome::Ridge: {
		float lo = (b == Biome::Gully || b == Biome::Ridge) ? 4.f : 2.f;
		sprinkle(lo, lo + 4.f, [&]() {
			float f0 = r(1500.f, 3000.f), f1 = f0 * r(1.1f, 1.5f);
			Buf a = tone(Wave::Sine, f0, f1, 0.08f, true);
			envAD(a, 0.005f, 0.075f);
			Buf c = tone(Wave::Sine, f1, f0 * r(0.9f, 1.2f), 0.1f, true);
			envAD(c, 0.005f, 0.095f);
			Buf e = concat(a, c);
			gain(e, 0.06f);
			return e;
		});
		break;
	}
	case Biome::River: {
		Buf w = noise(L + 0.5f, 61);
		lowpass(w, 900.f);
		normalize(w, 0.15f);
		tremolo(w, 0.25f, 0.4f);
		mix(out, seamless(w, 0.5f), 1.f);
		break;
	}
	case Biome::Hollow:
		sprinkle(3.f, 7.f, [&]() {
			Buf d = sineNote(1800.f, 0.03f, 0.001f, 0.029f);
			Buf e(samples(0.25f), 0.f);
			mix(e, d, 0.12f);
			mix(e, d, 0.05f, samples(0.2f));
			return e;
		});
		break;
	case Biome::Swamp:
		sprinkle(4.f, 9.f, [&]() {
			Buf e(samples(0.4f), 0.f);
			for (int i = 0; i < 3; ++i) {
				Buf rb = tone(Wave::Square, 140.f, 120.f, 0.08f, false, 0.3f);
				lowpass(rb, 800.f);
				envAD(rb, 0.005f, 0.075f);
				mix(e, rb, 0.07f, samples(0.11f * i));
			}
			return e;
		});
		sprinkle(2.f, 5.f, [&]() {
			Buf bz = tone(Wave::Square, 4000.f, 4100.f, 0.4f, false, 0.5f);
			tremolo(bz, 30.f, 0.9f);
			envADSR(bz, 0.1f, 0.1f, 0.8f, 0.15f);
			gain(bz, 0.01f);
			return bz;
		});
		break;
	case Biome::Mill:
		sprinkle(5.f, 10.f, [&]() {
			Buf c = tone(Wave::Saw, 140.f, 150.f, 0.5f);
			vibratoPitch(c, 4.f, 0.06f);
			lowpass(c, 700.f);
			envADSR(c, 0.05f, 0.1f, 0.7f, 0.2f);
			gain(c, 0.05f);
			return c;
		});
		for (float t = 0.f; t < L; t += 1.3f * (L / (std::floor(L / 1.3f) * 1.3f))) {
			Buf th = tone(Wave::Sine, 70.f, 55.f, 0.12f);
			envAD(th, 0.004f, 0.11f);
			mix(out, th, 0.08f, samples(t), true);
		}
		break;
	default:
		break;
	}
	return out;
}

// ---- generative marimba music ----
Buf buildMusic(Biome b) {
	struct Key {
		int root;
		bool minor;
	};
	const Key keys[] = {{60, false}, {62, false}, {57, true}, {55, false}, {52, true},
	                    {53, false}, {62, false}, {57, true}, {55, false}, {52, true}};
	Key k = keys[static_cast<int>(b)];
	float bpm = b == Biome::Storm ? 120.f : 96.f;
	float beat = 60.f / bpm;
	const int beats = 32;
	float L = beats * beat;
	Buf out(samples(L + 1.f), 0.f);
	Rng r(static_cast<unsigned>(b) * 7u + 101u);
	const int majP[5] = {0, 2, 4, 7, 9}, minP[5] = {0, 3, 5, 7, 10};
	const int* pent = k.minor ? minP : majP;
	const int majC[4] = {0, 9, 5, 7}, minC[4] = {0, 8, 5, 7};
	const int* chords = k.minor ? minC : majC;
	int step = 5; // index into a 2-octave pentatonic ladder (0..9)
	for (int i = 0; i < beats; ++i) {
		float t = i * beat;
		int chord = chords[(i / 2) % 4];
		if (i % 2 == 0) {
			// bass on the chord root + a soft stab
			int root = k.root - 12 + chord;
			if (root > k.root - 5) root -= 12;
			Buf bass = tone(Wave::Sine, noteHz(root), noteHz(root), beat * 2.f);
			envAD(bass, 0.01f, beat * 2.f, true);
			mix(out, bass, 0.22f, samples(t));
			bool minorChord = k.minor ? (chord == 0 || chord == 5 || chord == 7) : (chord == 9);
			int third = minorChord ? 3 : 4;
			for (int iv : {0, third, 7}) {
				Buf st = tone(Wave::Sine, noteHz(k.root + chord + iv), noteHz(k.root + chord + iv), beat * 1.5f);
				envAD(st, 0.02f, beat * 1.5f, true);
				mix(out, st, 0.03f, samples(t));
			}
		}
		if (r(0.f, 1.f) < 0.6f) {
			step = std::clamp(step + static_cast<int>(std::round(r(-2.4f, 2.4f))), 0, 9);
			int midi = k.root + 12 * (step / 5) + pent[step % 5];
			Buf n = marimba(noteHz(midi), 0.5f);
			mix(out, n, 0.16f, samples(t));
		}
		if (r(0.f, 1.f) < 0.15f) { // occasional off-beat grace note
			int midi = k.root + 12 * (step / 5) + pent[(step + 1) % 5];
			Buf n = marimba(noteHz(midi), 0.4f);
			mix(out, n, 0.08f, samples(t + beat * 0.5f));
		}
	}
	// wrap the tail into the head for a seamless loop
	Buf loop(out.begin(), out.begin() + samples(L));
	for (int i = samples(L); i < static_cast<int>(out.size()); ++i) loop[i - samples(L)] += out[i];
	return loop;
}

Buf noiseLoop(float cutoff, unsigned seed, bool brown = false) {
	Buf b = brown ? brownNoise(3.f, seed) : noise(3.f, seed);
	lowpass(b, cutoff);
	if (cutoff < 400.f) lowpass(b, cutoff);
	normalize(b, 0.9f);
	return seamless(b, 0.4f);
}

} // namespace

Audio::Audio(bool enabled) : myEnabled(enabled) {
	if (!myEnabled) return;
	std::vector<Buf> sfx = buildSfx();
	bufs.resize(sfx.size());
	for (size_t i = 0; i < sfx.size(); ++i) {
		auto pcm = toPCM(sfx[i].empty() ? Buf(64, 0.f) : sfx[i]);
		bufs[i].loadFromSamples(pcm.data(), pcm.size(), 1, SR);
	}
	for (int i = 0; i < 16; ++i) pool.push_back(std::make_unique<sf::Sound>());
	makeLoop(windLo, noiseLoop(200.f, 71));
	makeLoop(windHi, noiseLoop(1500.f, 73));
	makeLoop(crate, noiseLoop(400.f, 79));
	makeLoop(boulder, noiseLoop(300.f, 83, true));
	{
		Buf u = noise(3.f, 89);
		bandpass(u, 1000.f, 1400.f);
		bandpass(u, 1000.f, 1400.f);
		normalize(u, 0.9f);
		makeLoop(updraft, seamless(u, 0.4f));
	}
}

Audio::~Audio() {
	for (auto& s : pool)
		if (s) s->stop();
	stopLoops();
}

void Audio::makeLoop(Loop& l, const std::vector<float>& samplesF) {
	if (l.snd) l.snd->stop();
	auto pcm = toPCM(samplesF);
	if (!l.buf) l.buf = std::make_unique<sf::SoundBuffer>();
	l.buf->loadFromSamples(pcm.data(), pcm.size(), 1, SR);
	if (!l.snd) l.snd = std::make_unique<sf::Sound>();
	l.snd->setBuffer(*l.buf);
	l.snd->setLoop(true);
	l.snd->setVolume(0.f);
	l.vol = 0.f;
}

void Audio::driveLoop(Loop& l, float dt, float rate) {
	if (!l.snd) return;
	l.vol += (l.target - l.vol) * std::min(1.f, rate * dt);
	if (l.vol < 0.002f && l.target <= 0.f) {
		if (l.snd->getStatus() == sf::Sound::Playing) l.snd->pause();
		l.vol = 0.f;
		return;
	}
	if (l.snd->getStatus() != sf::Sound::Playing) l.snd->play();
	l.snd->setVolume(100.f * master * std::clamp(l.vol, 0.f, 1.f));
}

void Audio::stopLoops() {
	for (Loop* l : {&ambience, &music, &windLo, &windHi, &crate, &boulder, &updraft}) {
		if (l->snd) l->snd->stop();
		l->vol = l->target = 0.f;
	}
}

void Audio::setVolume(float m) { master = std::clamp(m, 0.f, 1.f); }

void Audio::play(Sfx s, float volume, float pitch) {
	if (!myEnabled) return;
	sf::Sound& snd = *pool[next];
	next = (next + 1) % pool.size();
	snd.stop();
	snd.setBuffer(bufs[static_cast<size_t>(s)]);
	snd.setVolume(std::clamp(100.f * volume * master, 0.f, 100.f));
	snd.setPitch(pitch);
	snd.play();
}

void Audio::setBiome(Biome b) {
	if (!myEnabled || b == biome) return;
	biome = b;
	makeLoop(ambience, buildAmbience(b));
	makeLoop(music, buildMusic(b));
}

void Audio::update(const World* w, float dt, float musicLevel) {
	if (!myEnabled) return;
	ambience.target = biome == Biome::Count ? 0.f : 1.f;
	music.target = musicOn ? 0.9f * musicLevel : 0.f;
	windLo.target = windHi.target = crate.target = boulder.target = updraft.target = 0.f;
	if (w) {
		bool windy = w->level.biome == Biome::Cliffs || w->level.biome == Biome::Storm || w->player.inWind || w->player.inUpdraft;
		if (windy) {
			float env = w->level.gust ? w->windEnvelope : 0.75f;
			windHi.target = 0.35f * env;
			windLo.target = 0.08f + 0.1f * (1.f - env);
		}
		crate.target = 0.15f * std::min(1.f, w->cratePush) * 2.f;
		boulder.target = 0.4f * w->boulderRoll;
		updraft.target = w->player.inUpdraft ? 0.15f : 0.f;
	}
	driveLoop(ambience, dt, 1.5f);
	driveLoop(music, dt, 1.2f);
	driveLoop(windLo, dt, 3.f);
	driveLoop(windHi, dt, 3.f);
	driveLoop(crate, dt, 10.f);
	driveLoop(boulder, dt, 8.f);
	driveLoop(updraft, dt, 5.f);
}

void Audio::handleEvents(const EventList& ev) {
	if (!myEnabled) return;
	for (const GameEvent& e : ev) {
		switch (e.type) {
		case Ev::Jump: play(Sfx::Jump, 1.f, e.value > 1.05f ? 1.1f : 1.f); break;
		case Ev::LandSoft: play(Sfx::LandSoft, 1.f); break;
		case Ev::LandHard: play(Sfx::LandHard, 1.f); break;
		case Ev::Step:
			stepPitchSeed = std::fmod(stepPitchSeed * 7.13f + 0.37f, 1.f);
			if (e.value > 0.5f) play(Sfx::StepWood, 1.f, 0.95f + 0.1f * stepPitchSeed);
			else play(Sfx::Step, 1.f, 0.9f + 0.2f * stepPitchSeed);
			break;
		case Ev::Banana:
			play(static_cast<Sfx>(static_cast<int>(Sfx::Banana0) + std::clamp(static_cast<int>(e.value), 0, 7)));
			break;
		case Ev::Fig: play(Sfx::Fig); break;
		case Ev::RopeGrab: play(Sfx::RopeGrab); break;
		case Ev::RopeRelease: play(Sfx::RopeRelease); break;
		case Ev::ClimbTick: play(Sfx::ClimbTick); break;
		case Ev::Splash: play(Sfx::Splash); break;
		case Ev::Drip: play(Sfx::Drip); break;
		case Ev::Death: play(Sfx::Hurt); break;
		case Ev::Checkpoint: play(Sfx::Checkpoint); break;
		case Ev::LevelComplete: play(Sfx::LevelComplete); break;
		case Ev::Bounce: play(Sfx::Boing, 1.f, e.value); break;
		case Ev::Squish: play(Sfx::Squish); break;
		case Ev::CrumbleShake: play(Sfx::CrumbleShake); break;
		case Ev::CrumbleFall: play(Sfx::CrumbleCrack); break;
		case Ev::LogCreak: play(Sfx::LogCreak); break;
		case Ev::LogThud: play(Sfx::LogThud); break;
		case Ev::BoulderImpact:
			if (e.value > 3.f) play(Sfx::BoulderImpact, std::min(1.f, e.value / 8.f));
			break;
		case Ev::GustWarn: play(Sfx::GustWhoosh, 0.8f); break;
		default: break;
		}
	}
}
