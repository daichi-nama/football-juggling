cmake_minimum_required(VERSION 3.1.0)

project(football-juggling CXX)

set(OpenGL_GL_PREFERENCE LEGACY)
find_package(OpenGL REQUIRED)

add_executable(football-juggling src/main.cpp)
target_include_directories(football-juggling
                           PRIVATE ${CMAKE_SOURCE_DIR}/external)
target_link_libraries(football-juggling PRIVATE ${OPENGL_LIBRARIES} glfw)
set_target_properties(football-juggling PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                                   "${CMAKE_BINARY_DIR}")
