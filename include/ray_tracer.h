#ifndef RAY_TRACER_H
#define RAY_TRACER_H

#include "util.h"
#include "mvl/mvl.h"
#include "mvl/affine.h"

/*
 * Performs ray tracing and renders to a texture. Displays rendered texture on the screen.
 */
class RayTracer
{
public:

	RayTracer();
	~RayTracer();

	// Render texture to screen
	void render();

	// Perform ray tracing on the scene
	void trace();	

private:

	// Initialization functions
	void init_quad();
	void init_texture();

	// Quad
	VFShaderProgram quad_program;
	GLuint quad_vao, quad_vbo, quad_uvb;
	GLuint uniform_quad_sampler, uniform_quad_mvp;
	aff::HomMatrix<GLfloat,3> quad_mvp;

	// Texture / Framebuffer rendering
	const GLuint texture_size_x = 800;
	const GLuint texture_size_y = 800;
	Texture texture;

	// Ray trace
	ComputeShaderProgram tracer_program;
	GLuint uniform_image_output;
};

#endif
