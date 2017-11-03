cmake_minimum_required(VERSION 3.8)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(ewig)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
set(CMAKE_CXX_EXTENSIONS off)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

set(CMAKE_CURSES_NEED_WIDE true)

find_package(Curses REQUIRED)
find_package(Boost 1.56 REQUIRED system)
find_package(Threads)
find_package(Immer)
find_package(Lager)
find_package(LibHttpServer)
find_path(SCELTA_INCLUDE_DIR scelta.hpp)
find_path(UTFCPP_INCLUDE_DIR utf8.h)

add_executable(ewig
  src/ewig/application.cpp
  src/ewig/buffer.cpp
  src/ewig/draw.cpp
  src/ewig/keys.cpp
  src/ewig/terminal.cpp
  src/ewig/main.cpp)
target_include_directories(ewig PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<INSTALL_INTERFACE:include>)
target_include_directories(ewig SYSTEM PUBLIC
  ${CURSES_INCLUDE_DIR}
  ${Boost_INCLUDE_DIR}
  ${SCELTA_INCLUDE_DIR}
  ${UTFCPP_INCLUDE_DIR}
  ${LIBHTTPSERVER_INCLUDE_DIRS})
target_link_libraries(ewig
  immer
  lager
  ${CURSES_LIBRARIES}
  ${Boost_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${LIBHTTPSERVER_LIBRARIES})

install(TARGETS ewig DESTINATION bin)
