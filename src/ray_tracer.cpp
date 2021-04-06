#include "ray_tracer.h"
#include <iostream>

/*
 * RayTracer Constructor.
 *
 * Loads and compilers shader programs, then runs the trace compute shader to generate the
 * texture.
 */
RayTracer::RayTracer()
	: quad_program("../data/shaders/ray_tracer.v.glsl", "../data/shaders/ray_tracer.f.glsl"), 
	tracer_program("../data/shaders/ray_tracer.c.glsl")
{
	init_quad();
	init_texture();

	// Perform trace once, reuse the texture
	trace();
}

/*
 * RayTracer Deconstructor.
 *
 * Frees any OpenGL handles the RayTracer uses.
 */
RayTracer::~RayTracer()
{
	// Delete quad buffers
	glDeleteBuffers(1, &quad_vbo);
	glDeleteBuffers(1, &quad_uvb);
	glDeleteVertexArrays(1, &quad_vao);
}

/*
 * Initializes all data used to render the full-screen quad which the texture is applied to.
 */
void RayTracer::init_quad()
{
	// Retrieve screen position attribute
	GLuint pos = quad_program.get_attrib("pos");
	GLuint uv = quad_program.get_attrib("uv");
	uniform_quad_sampler = quad_program.get_uniform("mySampler");
	uniform_quad_mvp = quad_program.get_uniform("mvp");

	// Screen position data
	const GLfloat quad_vertex_data[] = {
		-1.0f, -1.0f,
		-1.0f,  1.0f,
		 1.0f, -1.0f,
		 1.0f,  1.0f,
	};

	// Texture UV mapping
	const GLfloat uv_buffer_data[] = {
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
	};

	// Generate screen vertex attribute object
	glGenVertexArrays(1, &quad_vao);
	glBindVertexArray(quad_vao);

	// Generate and bind screen buffer data
	glGenBuffers(1, &quad_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_data), quad_vertex_data, GL_STATIC_DRAW);
	glVertexAttribPointer(pos, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), (void*) 0);
	glEnableVertexAttribArray(pos);

	// Generate and bind uv buffer data
	glGenBuffers(1, &quad_uvb);
	glBindBuffer(GL_ARRAY_BUFFER, quad_uvb);
	glBufferData(GL_ARRAY_BUFFER, sizeof(uv_buffer_data), uv_buffer_data, GL_STATIC_DRAW);
	glVertexAttribPointer(uv, 2, GL_FLOAT, GL_FALSE, 2*sizeof(GLfloat), (void*) 0);
	glEnableVertexAttribArray(uv);

	// Build quad MVP matrix
	auto proj = aff::orthographic<GLfloat>(mvl::Vector<GLfloat,3>{-1.0f, -1.0f, 0.0f}, mvl::Vector<GLfloat,3>{1.0f, 1.0f, 2.0f});
	auto view = aff::lookAt(mvl::Vector<GLfloat,3>{0.0f, 0.0f, 1.0f}, mvl::Vector<GLfloat,3>{0.0f, 0.0f, 0.0f}, mvl::Vector<GLfloat,3>{0.0f, -1.0f, 0.0f});
	auto model = aff::identity<GLfloat, 3>();
	quad_mvp = proj * view * model;
}

/*
 * Render the quad to the screen, and the texture to the quad.
 */
void RayTracer::render()
{
	// Clear screen
	glClearColor(0, 0, 0, 1);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	// Use screen render program
	quad_program.useProgram();

	glUniformMatrix4fv(uniform_quad_mvp, 1, GL_FALSE, quad_mvp.toArray());
	
	glActiveTexture(GL_TEXTURE0);
	texture.bindTexture();
	glUniform1i(uniform_quad_sampler, 0);

	// Draw screen
	glBindVertexArray(quad_vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	// Unbind last used vao
	glBindVertexArray(0);
}

/*
 * Initializes the texture for rendering.
 */
void RayTracer::init_texture()
{
	// Bind texture
	glActiveTexture(GL_TEXTURE0);
	texture.bindTexture();

	// Set filtering and wrapping
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// Set texture to render to
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, texture_size_x, texture_size_y, 0, GL_RGBA, GL_FLOAT, NULL);
	glBindImageTexture(0, texture.getId(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16F);
}

/*
 * Traces the scene using the compute shader loaded by the ray tracer.
 * The compute shader outputs to a texture, which is what is actually drawn to the screen.
 */
void RayTracer::trace()
{
	// Bind ray tracer uniforms
	GLuint tr_eye = tracer_program.get_uniform("eye");
	GLuint tr_center = tracer_program.get_uniform("scene_center");
	GLuint tr_raw_up = tracer_program.get_uniform("raw_up");
	GLuint tr_screen_size = tracer_program.get_uniform("screen_size");
	GLuint tr_near = tracer_program.get_uniform("near");

	// Initialize uniforms for the scene
	mvl::Vector<GLfloat, 3> eye{1.5, 0, -3};
	mvl::Vector<GLfloat, 3> center{0, -2, 0};
	mvl::Vector<GLfloat, 3> up{0, 1, 0};
	mvl::Vector<GLfloat, 2> screen_size{2, 2};
	GLfloat near = 1;

	// Trace scene 
	tracer_program.useProgram();
	
	glUniform3fv(tr_eye, 1, eye.toArray());
	glUniform3fv(tr_center, 1, center.toArray());
	glUniform3fv(tr_raw_up, 1, up.toArray());
	glUniform2fv(tr_screen_size, 1, screen_size.toArray());
	glUniform1f(tr_near, near);

	glDispatchCompute(texture_size_x, texture_size_y, 1);

	// Wait for compute shader to finish
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

