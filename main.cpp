#include "World.h"

#include <Box2D/Box2D.h>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include <iostream>

using namespace std;


 volatile int speed_counter = 0;

  void increment_speed_counter()
  {
	 speed_counter++;
  }



int main(int argc, char** argv)
{
   chdir("/home/meros/Documents/projects/TheGame/TheGame");

   sf::RenderWindow App(sf::VideoMode(800, 600, 32), "SFML Window");

   World world;

   cout <<"hej" << endl;

   App.UseVerticalSync(true);
   App.SetFramerateLimit(50);


   while (!App.GetInput().IsKeyDown(sf::Key::Escape))
   {
	   sf::Event event;
	   App.GetEvent(event);

	   world.Update(App.GetInput());

	   App.Clear(sf::Color::White);
	   world.Draw(App);

	   App.Display();
   }

   return 0;
}
