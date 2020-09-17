cmake_minimum_required(VERSION 3.7)
project(zbnt_server)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Type of build" FORCE)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

find_package(Qt5 COMPONENTS Core Network REQUIRED)
find_library(libfdt "libfdt.so")

set(ZBNT_FALLBACK_VERSION "2.0.0-beta.2")
set(CMAKE_CXX_STANDARD 14)

option(ZYNQ_MODE      "Build for Zynq/ZynqMP devices" OFF)
option(USE_SANITIZERS "Compile with ASan and UBSan"   OFF)

set(PROFILE_PATH  "/etc/zbnt"              CACHE PATH "Location of profile configuration files")
set(FIRMWARE_PATH "/usr/lib/firmware/zbnt" CACHE PATH "Location of bitstream and device tree files (Zynq/ZynqMP)")
set(SYSFS_PATH    "/sys"                   CACHE PATH "Path where sysfs is mounted")
set(CONFIGFS_PATH "/sys/kernel/config"     CACHE PATH "Path where configfs is mounted")

if(USE_SANITIZERS)
	set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wall -fsanitize=address -fsanitize=undefined")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fpermissive -fsanitize=address -fsanitize=undefined")
else()
	set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wall")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fpermissive")
endif()

add_custom_target(
	gen_version ALL
	COMMAND "python3" "cmake/GenerateVersion.py" "${CMAKE_BINARY_DIR}" "${ZBNT_FALLBACK_VERSION}"
	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	BYPRODUCTS "${CMAKE_BINARY_DIR}/Version.txt" "${CMAKE_BINARY_DIR}/Version.cpp"
	COMMENT "Generating version files"
)

set(ZBNT_SERVER_SRC
	"src/Main.cpp"

	"src/DiscoveryServer.cpp"
	"src/DmaBuffer.cpp"
	"src/FdtUtils.cpp"
	"src/IrqThread.cpp"

	"src/ZbntServer.cpp"
	"src/ZbntTcpServer.cpp"
	"src/ZbntLocalServer.cpp"

	"src/AbstractDevice.cpp"
	"src/$<IF:$<BOOL:${ZYNQ_MODE}>,Axi,Pci>Device.cpp"

	"server-shared/src/MessageUtils.cpp"
	"server-shared/src/MessageReceiver.cpp"
	"${CMAKE_BINARY_DIR}/Version.cpp"

	"src/cores/AbstractCore.cpp"
	"src/cores/AxiDma.cpp"
	"src/cores/AxiMdio.cpp"
	"src/cores/FrameDetector.cpp"
	"src/cores/LatencyMeasurer.cpp"
	"src/cores/PrController.cpp"
	"src/cores/SimpleTimer.cpp"
	"src/cores/StatsCollector.cpp"
	"src/cores/TrafficGenerator.cpp"
)

set(ZBNT_SERVER_HDR
	"include/IrqThread.hpp"
)

set_source_files_properties("${CMAKE_BINARY_DIR}/Version.cpp" PROPERTIES GENERATED 1)

qt5_wrap_cpp(ZBNT_SERVER_SRC_MOC ${ZBNT_SERVER_HDR})

add_executable(zbnt_server ${ZBNT_SERVER_SRC} ${ZBNT_SERVER_SRC_MOC})
add_dependencies(zbnt_server gen_version)
target_link_libraries(zbnt_server Qt5::Core Qt5::Network ${libfdt} -lpthread)
target_include_directories(zbnt_server PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/server-shared/include" "${CMAKE_BINARY_DIR}")

target_compile_definitions(
	zbnt_server PUBLIC
	ZBNT_ZYNQ_MODE=$<IF:$<BOOL:${ZYNQ_MODE}>,1,0>
	ZBNT_PROFILE_PATH="${PROFILE_PATH}"
	ZBNT_FIRMWARE_PATH="${FIRMWARE_PATH}"
	ZBNT_SYSFS_PATH="${SYSFS_PATH}"
	ZBNT_CONFIGFS_PATH="${CONFIGFS_PATH}"
)

install(
	TARGETS zbnt_server
	PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
	DESTINATION bin
)
