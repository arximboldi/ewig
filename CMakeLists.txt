cmake_minimum_required(VERSION 2.6)
project(ewig)

set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)

include_directories(
  ${CURSES_INCLUDE_DIR}
  "${PROJECT_SOURCE_DIR}/src"
  "${PROJECT_SOURCE_DIR}/deps/immer")

add_executable(ewig
  src/main.cpp
  src/ewig/main.cpp)
target_link_libraries(ewig ${CURSES_LIBRARIES})
