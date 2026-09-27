// Leafwind -- keyboard + gamepad polling into Input snapshots and menu edges (DESIGN 7.2).
#pragma once

#include "core/Input.h"

#include <SFML/Window.hpp>

struct MenuInput {
	bool up = false, down = false, left = false, right = false;
	bool confirm = false, back = false, pause = false, restart = false;
	bool any() const { return up || down || left || right || confirm || back || pause || restart; }
};

class InputDevice {
public:
	void handleEvent(const sf::Event& e);
	// Poll held state; consumes the jump/restart edges gathered since the last call.
	Input poll(bool focused);
	// Consume menu edges.
	MenuInput menu();
	void clear();

private:
	void pollJoystick();
	MenuInput edges;
	bool jumpEdge = false, restartEdge = false;
	bool joyPrev[8] = {false};
	int joyDirPrevX = 0, joyDirPrevY = 0;
};
