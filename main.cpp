#include "World.h"

#include <Box2D/Box2D.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <iostream>

using namespace std;

volatile int speed_counter = 0;

void increment_speed_counter() {
	speed_counter++;
}

int main(int argc, char** argv) {
	sf::RenderWindow app(sf::VideoMode(800, 600, 32), "SFML Window");

	World world;

	cout << "hej" << endl;

	app.setVerticalSyncEnabled(true);
	app.setFramerateLimit(50);

	while (true) {
		sf::Event event;
		bool gotEvent = app.pollEvent(event);

		if (gotEvent
				&& ((event.type == sf::Event::KeyPressed)
						&& (event.key.code == sf::Keyboard::Escape))) {
			break;
		}

		world.Update();

		app.clear(sf::Color::White);
		world.Draw(app);
		app.display();
	}

	return 0;
}
