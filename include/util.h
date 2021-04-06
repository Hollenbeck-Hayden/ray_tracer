#ifndef UTIL_H
#define UTIL_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>

#include <GL/glew.h>
#include "SDL.h"

#include <vector>
#include "mvl/mvl.h"

/*
 * A wrapper exception for OpenGL related exceptions.
 */
class OglException : public std::runtime_error
{
public:
	OglException(const std::string& what_arg) : std::runtime_error(what_arg) {}
};


/*
 * Print runtime information about OpenGL.
 */
void print_opengl_info();

/*
 * Initialize GLEW.
 */
void initGlew();

/*
 * Handles an SDL window.
 */
class Window
{
public:
	Window(const std::string& title, int width, int height);
	~Window();

	int getWidth();
	int getHeight();
	void swapBuffers();

private:
	SDL_Window* window;
};

/*
 * Handles a shader program. Wraps OpenGL calls for easier / safer use.
 * May be created from vertex and fragment shaders, or compute shaders.
 */
class ShaderProgram
{
public:
	ShaderProgram();
	~ShaderProgram();

	// Get shader layout handles
	GLint get_attrib(const std::string& name);
	GLint get_uniform(const std::string& name);

	// Binds program for OpenGL use
	void useProgram();

protected:
	GLuint program;

	// Helper functions for creating a shader
	GLuint create_shader(const std::string& filename, GLenum type);
	std::string read_file(const std::string& filename);
	void print_log(GLuint object, std::ostream& out);
	void linkProgram();
};

/*
 * A vertex and fragment shader program.
 * Same as ShaderProgram except explicitly built form a vertex and fragment shader.
 */
class VFShaderProgram : public ShaderProgram
{
public:
	VFShaderProgram(const std::string& vs_filename, const std::string& fs_filename);
};

/*
 * A compute shader program.
 * Same as ShaderProgram except explicitly built from a compute shader.
 */
class ComputeShaderProgram : public ShaderProgram
{
public:
	ComputeShaderProgram(const std::string& filename);
};

/*
 * Handles a texture. Wraps OpenGL calls for safer / easier use.
 */
class Texture
{
public:
	~Texture();

	void bindTexture();
	GLuint getId();

private:
	GLuint texture;
};

#endif
