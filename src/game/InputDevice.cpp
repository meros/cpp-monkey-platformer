#include "InputDevice.h"

#include <cmath>

using K = sf::Keyboard;

void InputDevice::handleEvent(const sf::Event& e) {
	if (e.type != sf::Event::KeyPressed) return;
	switch (e.key.code) {
	case K::Up:
	case K::W: edges.up = true; break;
	case K::Down:
	case K::S: edges.down = true; break;
	case K::Left:
	case K::A: edges.left = true; break;
	case K::Right:
	case K::D: edges.right = true; break;
	case K::Space:
	case K::Z:
	case K::K:
		edges.confirm = true;
		jumpEdge = true;
		break;
	case K::Enter: edges.confirm = true; break;
	case K::Escape:
		edges.back = true;
		edges.pause = true;
		break;
	case K::R:
		edges.restart = true;
		restartEdge = true;
		break;
	default: break;
	}
}

void InputDevice::pollJoystick() {
	if (!sf::Joystick::isConnected(0)) return;
	auto btn = [&](int b) { return sf::Joystick::getButtonCount(0) > static_cast<unsigned>(b) && sf::Joystick::isButtonPressed(0, b); };
	bool now[8];
	for (int b = 0; b < 8; ++b) now[b] = btn(b);
	if (now[0] && !joyPrev[0]) {
		edges.confirm = true;
		jumpEdge = true;
	}
	if (now[1] && !joyPrev[1]) edges.back = true;
	if (now[7] && !joyPrev[7]) {
		edges.pause = true;
		edges.back = true;
	}
	if (now[6] && !joyPrev[6]) {
		edges.restart = true;
		restartEdge = true;
	}
	for (int b = 0; b < 8; ++b) joyPrev[b] = now[b];
	float ax = sf::Joystick::getAxisPosition(0, sf::Joystick::X);
	float ay = sf::Joystick::getAxisPosition(0, sf::Joystick::Y);
	float px = sf::Joystick::getAxisPosition(0, sf::Joystick::PovX);
	float py = sf::Joystick::getAxisPosition(0, sf::Joystick::PovY);
	int dx = (ax < -50 || px < -50) ? -1 : (ax > 50 || px > 50) ? 1 : 0;
	int dy = (ay < -50 || py > 50) ? -1 : (ay > 50 || py < -50) ? 1 : 0;
	if (dx != joyDirPrevX) {
		if (dx < 0) edges.left = true;
		if (dx > 0) edges.right = true;
	}
	if (dy != joyDirPrevY) {
		if (dy < 0) edges.up = true;
		if (dy > 0) edges.down = true;
	}
	joyDirPrevX = dx;
	joyDirPrevY = dy;
}

Input InputDevice::poll(bool focused) {
	pollJoystick();
	Input in;
	if (focused) {
		bool l = K::isKeyPressed(K::Left) || K::isKeyPressed(K::A);
		bool r = K::isKeyPressed(K::Right) || K::isKeyPressed(K::D);
		in.up = K::isKeyPressed(K::Up) || K::isKeyPressed(K::W);
		in.down = K::isKeyPressed(K::Down) || K::isKeyPressed(K::S);
		in.jumpHeld = K::isKeyPressed(K::Space) || K::isKeyPressed(K::Z) || K::isKeyPressed(K::K);
		if (sf::Joystick::isConnected(0)) {
			float ax = sf::Joystick::getAxisPosition(0, sf::Joystick::X);
			float ay = sf::Joystick::getAxisPosition(0, sf::Joystick::Y);
			float px = sf::Joystick::getAxisPosition(0, sf::Joystick::PovX);
			float py = sf::Joystick::getAxisPosition(0, sf::Joystick::PovY);
			l = l || ax < -35 || px < -50;
			r = r || ax > 35 || px > 50;
			in.up = in.up || ay < -50 || py > 50;
			in.down = in.down || ay > 50 || py < -50;
			in.jumpHeld = in.jumpHeld || sf::Joystick::isButtonPressed(0, 0);
		}
		in.dir = (r ? 1 : 0) - (l ? 1 : 0);
	}
	in.jumpPressed = jumpEdge;
	in.restartPressed = restartEdge;
	jumpEdge = restartEdge = false;
	return in;
}

MenuInput InputDevice::menu() {
	pollJoystick();
	MenuInput m = edges;
	edges = MenuInput();
	return m;
}

void InputDevice::clear() {
	edges = MenuInput();
	jumpEdge = restartEdge = false;
}
