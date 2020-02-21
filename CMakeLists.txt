cmake_minimum_required(VERSION 3.7)
project(zbnt_sw VERSION "2.0.0")

option(BUILD_GUI      "Build the reference GUI"           ON)
option(BUILD_DAEMON   "Build the daemon for Zynq devices" OFF)
option(USE_SANITIZERS "Compile with ASan and UBSan"       OFF)

math(EXPR ZBNT_VERSION_INT "(${PROJECT_VERSION_MAJOR} << 24) | (${PROJECT_VERSION_MINOR} << 16) | ${PROJECT_VERSION_PATCH}")

set(CFG_PATH      "/etc/zbnt"              CACHE PATH "Location of configuration files")
set(FIRMWARE_PATH "/usr/lib/firmware/zbnt" CACHE PATH "Location of bitstream and device tree files")
set(SYSFS_PATH    "/sys"                   CACHE PATH "Path where sysfs is mounted")
set(CONFIGFS_PATH "/sys/kernel/config"     CACHE PATH "Path where configfs is mounted")

set(CMAKE_CXX_STANDARD 14)

if(USE_SANITIZERS)
	set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wall -fsanitize=address -fsanitize=undefined")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fpermissive -fsanitize=address -fsanitize=undefined")
else()
	set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wall")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fpermissive")
endif()

if(BUILD_GUI)
	add_subdirectory(gui)
endif()

if(BUILD_DAEMON)
	add_subdirectory(daemon)
endif()
