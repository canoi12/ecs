cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)

project(ecs VERSION 0.1.0 LANGUAGES C)

find_package(SDL2 REQUIRED)

include_directories(.)

add_executable(${PROJECT_NAME} examples/sdl2.c)
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})