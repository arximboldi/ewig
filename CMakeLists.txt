cmake_minimum_required(VERSION 3.8)
project(ewig)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

find_package(Boost 1.56 REQUIRED system)
find_package(Threads)
find_package(Curses REQUIRED)

include_directories(${CURSES_INCLUDE_DIR})

add_executable(ewig
  src/ewig/application.cpp
  src/ewig/buffer.cpp
  src/ewig/draw.cpp
  src/ewig/keys.cpp
  src/ewig/terminal.cpp
  src/ewig/main.cpp)
target_include_directories(ewig PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/deps/immer>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/deps/scelta/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/deps/utfcpp/source>
  $<INSTALL_INTERFACE:include>)
target_include_directories(ewig SYSTEM PUBLIC
  ${Boost_INCLUDE_DIR})
target_link_libraries(ewig
  ${CURSES_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS ewig DESTINATION bin)
