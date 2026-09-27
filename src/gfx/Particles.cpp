#include "Particles.h"

#include <algorithm>
#include <cmath>

Particles::Particles() { parts.reserve(kMax); }

Particle& Particles::spawn() {
	if (parts.size() >= kMax) {
		// recycle the oldest-looking particle
		size_t best = 0;
		for (size_t i = 1; i < parts.size(); ++i)
			if (parts[i].life / parts[i].maxLife > parts[best].life / parts[best].maxLife) best = i;
		parts[best] = Particle();
		return parts[best];
	}
	parts.emplace_back();
	return parts.back();
}

void Particles::update(float dt, float windX) {
	for (auto& p : parts) {
		p.life += dt;
		p.vel.y += p.gravity * dt;
		if (p.drag > 0.f) p.vel -= p.vel * std::min(1.f, p.drag * dt);
		V2 v = p.vel;
		if (p.kind == PKind::Leaf) {
			v.x += std::sin(p.life * 2.2f + p.phase) * p.wobble + windX;
		}
		if (p.kind == PKind::Firefly) {
			v.x += std::sin(p.life * 0.9f + p.phase) * 12.f;
			v.y += std::cos(p.life * 0.7f + p.phase * 1.3f) * 10.f;
		}
		p.pos += v * dt;
		p.rot += p.spin * dt;
	}
	parts.erase(std::remove_if(parts.begin(), parts.end(), [](const Particle& p) { return p.life >= p.maxLife; }),
	            parts.end());
}

void Particles::draw(sf::RenderTarget& t, const sf::Texture& atlas) {
	normal.clear();
	add.clear();
	for (const auto& p : parts) {
		float u = p.life / p.maxLife;
		float a = 1.f - u;
		if (p.fadeIn) a = std::min(a * 1.2f, std::min(1.f, p.life / 0.4f));
		a = std::clamp(a, 0.f, 1.f);
		sf::Color c = p.color;
		c.a = static_cast<sf::Uint8>(c.a * a);
		float s = p.shrink ? p.size * (0.4f + 0.6f * (1.f - u)) : p.size;
		SpriteBatch& b = p.additive ? add : normal;
		switch (p.kind) {
		case PKind::Dot:
			b.add(Tex::Glow, p.pos, V2(s, s), 0.f, c);
			break;
		case PKind::Drop:
			b.add(Tex::Drop, p.pos, V2(s, s), 0.f, c);
			break;
		case PKind::Leaf:
			b.add(Tex::Leaf, p.pos, V2(s, s * 0.6f), p.rot, c);
			break;
		case PKind::Star:
			b.add(Tex::Star, p.pos, V2(s, s), p.rot, c);
			break;
		case PKind::Streak: {
			V2 d = norm(p.vel);
			b.addStreak(p.pos - d * s, p.pos, std::max(1.f, p.size * 0.12f), c);
			break;
		}
		case PKind::Firefly: {
			float pulse = 0.75f + 0.25f * std::sin(p.life * 3.14159f + p.phase);
			sf::Color cc = c;
			cc.a = static_cast<sf::Uint8>(cc.a * pulse);
			b.add(Tex::Glow, p.pos, V2(s * 2.5f, s * 2.5f), 0.f, sf::Color(cc.r, cc.g, cc.b, cc.a / 3));
			b.add(Tex::Drop, p.pos, V2(s * 0.5f, s * 0.5f), 0.f, cc);
			break;
		}
		case PKind::Ring: {
			float r = p.size * (0.3f + 1.2f * u);
			const int N = 20;
			sf::FloatRect tr = atlasRect(Tex::Solid);
			V2 uv(tr.left + 2, tr.top + 2);
			for (int i = 0; i < N; ++i) {
				float a0 = 6.2831853f * i / N, a1 = 6.2831853f * (i + 1) / N;
				V2 d0(std::cos(a0), std::sin(a0) * 0.3f), d1(std::cos(a1), std::sin(a1) * 0.3f);
				float w = 1.5f;
				sf::Vertex v[4] = {sf::Vertex(p.pos + d0 * (r - w), c, uv), sf::Vertex(p.pos + d0 * (r + w), c, uv),
				                   sf::Vertex(p.pos + d1 * (r + w), c, uv), sf::Vertex(p.pos + d1 * (r - w), c, uv)};
				b.va.append(v[0]);
				b.va.append(v[1]);
				b.va.append(v[2]);
				b.va.append(v[0]);
				b.va.append(v[2]);
				b.va.append(v[3]);
			}
			break;
		}
		}
	}
	normal.draw(t, atlas, sf::BlendAlpha);
	add.draw(t, atlas, sf::BlendAdd);
}

void Particles::dust(V2 p, int n, float spread, sf::Color c) {
	for (int i = 0; i < n; ++i) {
		Particle& q = spawn();
		q.kind = PKind::Dot;
		q.pos = p + V2(rnd(-spread, spread), rnd(-3.f, 1.f));
		float a = rnd(-3.14159f, 0.f);
		float sp = rnd(20.f, 70.f);
		q.vel = V2(std::cos(a) * sp, std::sin(a) * sp * 0.5f);
		q.drag = 3.f;
		q.gravity = -10.f;
		q.maxLife = rnd(0.3f, 0.6f);
		q.size = rnd(4.f, 8.f);
		q.color = c;
	}
}

void Particles::sparkles(V2 p, int n, sf::Color c, float life, float speed) {
	for (int i = 0; i < n; ++i) {
		Particle& q = spawn();
		q.kind = PKind::Star;
		q.pos = p;
		float a = rnd(0.f, 6.2831f);
		float sp = rnd(speed * 0.4f, speed);
		q.vel = V2(std::cos(a) * sp, std::sin(a) * sp);
		q.drag = 4.f;
		q.maxLife = rnd(life * 0.6f, life);
		q.size = rnd(4.f, 8.f);
		q.spin = rnd(-4.f, 4.f);
		q.color = c;
		q.additive = true;
	}
}

void Particles::leafBurst(V2 p, int n, sf::Color a, sf::Color b) {
	for (int i = 0; i < n; ++i) {
		Particle& q = spawn();
		q.kind = PKind::Leaf;
		q.pos = p + V2(rnd(-8.f, 8.f), rnd(-8.f, 8.f));
		float ang = rnd(0.f, 6.2831f);
		float sp = rnd(60.f, 200.f);
		q.vel = V2(std::cos(ang) * sp, std::sin(ang) * sp - 60.f);
		q.gravity = 160.f;
		q.drag = 2.5f;
		q.wobble = 25.f;
		q.phase = rnd(0.f, 6.f);
		q.maxLife = rnd(0.8f, 1.4f);
		q.size = rnd(5.f, 8.f);
		q.rot = ang;
		q.spin = rnd(-6.f, 6.f);
		q.color = (i % 2) ? a : b;
		q.shrink = false;
	}
}

void Particles::splash(V2 p, int n) {
	for (int i = 0; i < n; ++i) {
		Particle& q = spawn();
		q.kind = PKind::Drop;
		q.pos = p + V2(rnd(-10.f, 10.f), 0.f);
		q.vel = V2(rnd(-120.f, 120.f), rnd(-320.f, -120.f));
		q.gravity = 900.f;
		q.maxLife = rnd(0.4f, 0.7f);
		q.size = rnd(2.f, 4.f);
		q.color = sf::Color(0xDC, 0xF5, 0xFF, 230);
		q.shrink = false;
	}
	ring(p, 30.f, sf::Color(0xDC, 0xF5, 0xFF, 200), 0.6f);
}

void Particles::spores(V2 p, int n, sf::Color c) {
	for (int i = 0; i < n; ++i) {
		Particle& q = spawn();
		q.kind = PKind::Dot;
		q.pos = p + V2(rnd(-16.f, 16.f), rnd(-4.f, 4.f));
		q.vel = V2(rnd(-50.f, 50.f), rnd(-110.f, -30.f));
		q.drag = 2.f;
		q.gravity = 20.f;
		q.maxLife = rnd(0.6f, 1.2f);
		q.size = rnd(3.f, 5.f);
		q.color = c;
		q.additive = true;
	}
}

void Particles::pebbles(V2 p, int n, sf::Color c) {
	for (int i = 0; i < n; ++i) {
		Particle& q = spawn();
		q.kind = PKind::Drop;
		q.pos = p + V2(rnd(-18.f, 18.f), 0.f);
		q.vel = V2(rnd(-20.f, 20.f), rnd(0.f, 40.f));
		q.gravity = 900.f;
		q.maxLife = rnd(0.4f, 0.8f);
		q.size = rnd(1.5f, 3.f);
		q.color = c;
		q.shrink = false;
	}
}

void Particles::ring(V2 p, float r0, sf::Color c, float life) {
	Particle& q = spawn();
	q.kind = PKind::Ring;
	q.pos = p;
	q.size = r0;
	q.maxLife = life;
	q.color = c;
	q.shrink = false;
}
