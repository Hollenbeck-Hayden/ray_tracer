cmake_minimum_required(VERSION 3.13)
set(program_target "ray_tracer")
project(${program_target} VERSION 0.0.1 LANGUAGES CXX)

# Define program target
add_executable(${program_target})
target_include_directories(${program_target} PUBLIC include)

# Add source directories
add_subdirectory(src)


# Add cmath library
target_link_libraries(${program_target} m)

# Add MVL library
find_package(mvl CONFIG REQUIRED)
target_link_libraries(${program_target} mvl::mvl)

# Add OpenGL library
set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL REQUIRED)
target_include_directories(${program_target} PUBLIC ${OPENGL_INCLUDE_DIRS})
target_link_libraries(${program_target} ${OPENGL_LIBRARIES})

# Add GLEW library
find_package(GLEW REQUIRED)
target_include_directories(${program_target} PUBLIC ${GLEW_INCLUDE_DIRS})
target_link_libraries(${program_target} ${GLEW_LIBRARIES})

# Add SDL library
set(SDL2_INCLUDE_DIR /usr/include/SDL2)
set(SDL2_LIBRARY /usr/lib/libSDL2.so)
target_include_directories(${program_target} PUBLIC ${SDL2_INCLUDE_DIR})
target_link_libraries(${program_target} ${SDL2_LIBRARY})
