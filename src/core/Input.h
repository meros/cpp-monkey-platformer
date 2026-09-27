// Leafwind -- per-frame input snapshot (docs/DESIGN.md 7.2). Filled by the keyboard /
// joystick poller in the game layer or by scripted inputs in tests.
#pragma once

struct Input {
	int dir = 0;              // -1 left, 0, +1 right
	bool up = false;
	bool down = false;
	bool jumpHeld = false;
	bool jumpPressed = false; // edge: pressed since last step
	bool restartPressed = false;

	void clearEdges() {
		jumpPressed = false;
		restartPressed = false;
	}
};
