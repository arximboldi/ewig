cmake_minimum_required(VERSION 2.6)
project(ewig)

find_package(PkgConfig)
find_package(SDL2 REQUIRED)
pkg_check_modules(SDL2_ttf REQUIRED SDL2_ttf)

include_directories(
  ${SDL2_INCLUDE_DIR}
  ${SDL2_ttf_INCLUDE_DIR}
  "${PROJECT_SOURCE_DIR}/../immer")

add_executable(ewig src/main.cpp)
target_link_libraries(ewig ${SDL2_LIBRARIES} ${SDL2_ttf_LIBRARIES})
