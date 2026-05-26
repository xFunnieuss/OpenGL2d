cmake_minimum_required(VERSION 4.2)
project(openglfun)

set(CMAKE_CXX_STANDARD 20)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

add_executable(openglfun main.cpp)

target_link_libraries(openglfun PRIVATE glfw OpenGL::GL)