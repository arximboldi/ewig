cmake_minimum_required(VERSION 2.6)
project(ewig)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

find_package(Boost 1.56 REQUIRED)

include_directories(
  ${Boost_INCLUDE_DIR}
  "${PROJECT_SOURCE_DIR}/src"
  "${PROJECT_SOURCE_DIR}/deps/immer")

add_executable(ewig
  src/ewig/application.cpp
  src/ewig/buffer.cpp
  src/ewig/draw.cpp
  src/ewig/keys.cpp
  src/ewig/tui.cpp
  src/main.cpp
  )
target_link_libraries(ewig -lncursesw)
