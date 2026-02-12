#include "World.h"

#include <box2d/box2d.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <cstring>
#include <iostream>
#include <string>

using namespace std;

static int runGame(const string& screenshotPath, int screenshotAfterFrames) {
	sf::RenderWindow app(sf::VideoMode(800, 600, 32), "Monkey Platformer");

	World world;

	app.setVerticalSyncEnabled(true);
	app.setFramerateLimit(50);

	int frameCount = 0;

	while (app.isOpen()) {
		sf::Event event;
		while (app.pollEvent(event)) {
			if (event.type == sf::Event::Closed) {
				app.close();
			}
			if (event.type == sf::Event::KeyPressed
					&& event.key.code == sf::Keyboard::Escape) {
				app.close();
			}
		}

		world.Update();

		app.clear(sf::Color::White);
		world.Draw(app);
		app.display();

		frameCount++;

		if (!screenshotPath.empty() && frameCount >= screenshotAfterFrames) {
			sf::Texture texture;
			texture.create(app.getSize().x, app.getSize().y);
			texture.update(app);
			if (texture.copyToImage().saveToFile(screenshotPath)) {
				cout << "Screenshot saved to: " << screenshotPath << endl;
			} else {
				cerr << "Failed to save screenshot to: " << screenshotPath << endl;
				return 1;
			}
			app.close();
		}
	}

	return 0;
}

static int runPhysicsTest() {
	World world;

	Point startPos = world.GetPlayerPosition();
	cout << "Initial player position: (" << startPos.myX << ", " << startPos.myY << ")" << endl;

	// The level is roughly 16 rows of 0.20 blocks = 3.2 world units tall
	// Player should never fall below the level bounds
	const float MAX_Y = 10.0f;
	const int SIM_FRAMES = 500;  // 10 seconds at 50fps

	float minY = startPos.myY;
	float maxY = startPos.myY;
	bool positionChanged = false;

	for (int frame = 0; frame < SIM_FRAMES; frame++) {
		world.Update();

		Point pos = world.GetPlayerPosition();

		if (pos.myY < minY) minY = pos.myY;
		if (pos.myY > maxY) maxY = pos.myY;

		if (pos.myX != startPos.myX || pos.myY != startPos.myY) {
			positionChanged = true;
		}

		// Check: player hasn't fallen through the world
		if (pos.myY > MAX_Y) {
			cerr << "FAIL: Player fell through the ground at frame " << frame
				 << " (Y=" << pos.myY << " > " << MAX_Y << ")" << endl;
			return 1;
		}

		// Check: position is not NaN (physics explosion)
		if (pos.myX != pos.myX || pos.myY != pos.myY) {
			cerr << "FAIL: Player position is NaN at frame " << frame << endl;
			return 1;
		}
	}

	cout << "After " << SIM_FRAMES << " frames:" << endl;
	Point finalPos = world.GetPlayerPosition();
	cout << "  Final position: (" << finalPos.myX << ", " << finalPos.myY << ")" << endl;
	cout << "  Y range: [" << minY << ", " << maxY << "]" << endl;

	// Check: physics simulation actually did something (player should fall due to gravity)
	if (!positionChanged) {
		cerr << "FAIL: Player position never changed (physics stuck)" << endl;
		return 1;
	}

	cout << "PASS: Player didn't fall through the ground" << endl;
	cout << "PASS: Player position changed (physics working)" << endl;
	cout << "PASS: No NaN positions (physics stable)" << endl;

	return 0;
}

int main(int argc, char** argv) {
	string screenshotPath;
	int screenshotAfterFrames = 0;
	bool testPhysics = false;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--screenshot") == 0 && i + 2 < argc) {
			screenshotPath = argv[i + 1];
			screenshotAfterFrames = atoi(argv[i + 2]);
			i += 2;
		} else if (strcmp(argv[i], "--test-physics") == 0) {
			testPhysics = true;
		}
	}

	if (testPhysics) {
		return runPhysicsTest();
	}

	return runGame(screenshotPath, screenshotAfterFrames);
}
