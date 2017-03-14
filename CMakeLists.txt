cmake_minimum_required(VERSION 2.6)
project(ewig)

find_package(SDL2 REQUIRED)
include_directories(
  ${SDL2_INCLUDE_DIR}
  "${PROJECT_SOURCE_DIR}/../immer")

add_executable(ewig src/main.cpp)
target_link_libraries(ewig ${SDL2_LIBRARIES})
