cmake_minimum_required(VERSION 3.7)
project(zbnt_sw)

option(BUILD_GUI "Build the reference GUI" ON)
option(BUILD_DAEMON "Build the daemon for Zynq devices" OFF)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

if(BUILD_GUI)
	add_subdirectory(gui)
endif()

if(BUILD_DAEMON)
	add_subdirectory(daemon)
endif()
