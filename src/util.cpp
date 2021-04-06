#include "util.h"
#include <stdio.h>

//	----- Utility -----

void print_opengl_info() {
	int major, minor, profile;
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profile);

	std::cout << "OpenGL " << major << "." << minor << " ";

	if (profile & SDL_GL_CONTEXT_PROFILE_CORE)
		std::cout << "CORE";
	if (profile & SDL_GL_CONTEXT_PROFILE_COMPATIBILITY)
		std::cout << "COMPATIBILITY";
	if (profile & SDL_GL_CONTEXT_PROFILE_ES)
		std::cout << "ES";
	
	std::cout << std::endl;
}

void initGlew() {
	GLenum glew_status = glewInit();
	if (glew_status != GLEW_OK)
	{
		std::stringstream error_log;
		error_log << "Error: glewInit: " << glewGetErrorString(glew_status);
		throw OglException(error_log.str());
	}
}

//	----- Window -----

Window::Window(const std::string& title, int width, int height) {
	window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
					width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

	if (window == nullptr)
	{
		std::stringstream error_log;
		error_log << "Could not create window: " << SDL_GetError();
		throw OglException(error_log.str());
	}

	SDL_GL_CreateContext(window);
	initGlew();
}

Window::~Window() {
	SDL_DestroyWindow(window);
}

int Window::getWidth() {
	int width;
	SDL_GetWindowSize(window, &width, nullptr);
	return width;
}

int Window::getHeight() {
	int height;
	SDL_GetWindowSize(window, nullptr, &height);
	return height;
}

void Window::swapBuffers() {
	SDL_GL_SwapWindow(window);
}

//	----- Shader -----

ShaderProgram::ShaderProgram()
{
	program = glCreateProgram();
}

ShaderProgram::~ShaderProgram() {
	glDeleteProgram(program);
}

GLint ShaderProgram::get_attrib(const std::string& name)
{
	GLint attribute = glGetAttribLocation(program, name.c_str());
	if (attribute < 0)
		throw OglException("Could not bind attribute " + name);
	return attribute;
}

GLint ShaderProgram::get_uniform(const std::string& name)
{
	GLint uniform = glGetUniformLocation(program, name.c_str());
	if (uniform < 0)
		throw OglException("Could not bind attribute " + name);
	return uniform;
}

void ShaderProgram::useProgram() {
	glUseProgram(program);
}

GLuint ShaderProgram::create_shader(const std::string& filename, GLenum type) {
	std::string source = read_file(filename);

	const char* version = "#version 430\n";

	const GLchar* sources[] = {
		version,
		source.c_str()
	};

	GLuint res = glCreateShader(type);
	glShaderSource(res, 2, sources, NULL);

	glCompileShader(res);
	GLint compile_ok = GL_FALSE;
	glGetShaderiv(res, GL_COMPILE_STATUS, &compile_ok);

	if (compile_ok == GL_FALSE) {
		std::stringstream error_log;
		error_log << filename << ": ";
		print_log(res, error_log);
		glDeleteShader(res);
		throw OglException(error_log.str());
	}

	return res;
}

std::string ShaderProgram::read_file(const std::string& filename) {
	std::stringstream buf;
	
	try {
		std::ifstream infile(filename);

		std::string line;
		while (std::getline(infile, line))
		{
			buf << line << std::endl;
		}

		infile.close();

	} catch(std::ios_base::failure& e) {
		std::cerr << "Couldn't read file: " << filename << std::endl;
		std::cerr << e.what() << std::endl;
	}

	return buf.str();
}

void ShaderProgram::linkProgram() {
	glLinkProgram(program);
	GLint link_ok = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &link_ok);
	if (link_ok == GL_FALSE) {
		std::stringstream error_log;
		print_log(program, error_log);
		glDeleteProgram(program);
		throw OglException(error_log.str());
	}
}

void ShaderProgram::print_log(GLuint object, std::ostream& out) {
	GLint log_length = 0;

	if (glIsShader(object))
		glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);
	else if (glIsProgram(object))
		glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_length);
	else {
		out << "print_log: Not a shader or program" << std::endl;
		return;
	}
	
	char* log = new char[log_length];
	if (glIsShader(object))
		glGetShaderInfoLog(object, log_length, NULL, log);
	else if (glIsProgram(object))
		glGetProgramInfoLog(object, log_length, NULL, log);

	std::string log_str(log);
	out << log_str << std::endl;

	delete[] log;
}

//	----- VFShaderProgram -----

VFShaderProgram::VFShaderProgram(const std::string& vs_filename, const std::string& fs_filename)
{
	GLuint vs_shader = create_shader(vs_filename, GL_VERTEX_SHADER);
	glAttachShader(program, vs_shader);

	GLuint fs_shader = create_shader(fs_filename, GL_FRAGMENT_SHADER);
	glAttachShader(program, fs_shader);


	linkProgram();
}

//	----- ComputeShaderProgram -----

ComputeShaderProgram::ComputeShaderProgram(const std::string& filename)
{
	GLuint compute_shader = create_shader(filename, GL_COMPUTE_SHADER);
	glAttachShader(program, compute_shader);
	linkProgram();
}

//	----- Texture -----
Texture::~Texture()
{
	glDeleteTextures(1, &texture);
}

void Texture::bindTexture()
{
	glBindTexture(GL_TEXTURE_2D, texture);
}

GLuint Texture::getId()
{
	return texture;
}
