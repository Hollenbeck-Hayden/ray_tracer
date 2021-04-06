#include <GL/glew.h>
#include "SDL.h"

#include "util.h"
#include "ray_tracer.h"


/*
 * Listens for SDL Events and dispatches them.
 * Returns true to continue, false to quit.
 */
bool pollEvents()
{
	SDL_Event ev;
	while (SDL_PollEvent(&ev))
	{
		switch (ev.type)
		{
			case SDL_QUIT:
				// Quit if program is exited.
				return false;
			
		};
	}

	return true;
}

/*
 * Runs the program.
 */
void run()
{
	// Create a window
	Window window("Test", 640, 480);

	// Create a ray tracer
	RayTracer tracer;

	// Extra OpenGL settings
	print_opengl_info();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// Main loop
	while (pollEvents())
	{
		// Render ray trace results
		tracer.render();

		// Refresh window
		window.swapBuffers();
	}
}

/*
 * Main function.
 */
int main(int argc, char** argv)
{
	// Start SDL
	SDL_Init(SDL_INIT_VIDEO);

	// Run the program
	run();

	// Quit SDL
	SDL_Quit();

	return 0;
}
