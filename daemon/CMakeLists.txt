
find_package(Qt5 COMPONENTS Core Network REQUIRED)
find_library(libfdt "libfdt.so")

set(ZBNT_DAEMON_SRC
	"src/Main.cpp"
	"src/FdtUtils.cpp"
	"src/Settings.cpp"
	"src/DiscoveryServer.cpp"
	"src/BitstreamManager.cpp"
	"src/MeasurementServer.cpp"

	"../common/src/Utils.cpp"
	"../common/src/MessageReceiver.cpp"
	"${CMAKE_BINARY_DIR}/Version.cpp"

	"src/dev/AbstractDevice.cpp"
	"src/dev/AxiDma.cpp"
	"src/dev/AxiMdio.cpp"
	"src/dev/DmaBuffer.cpp"
	"src/dev/SimpleTimer.cpp"
	"src/dev/FrameDetector.cpp"
	"src/dev/StatsCollector.cpp"
	"src/dev/LatencyMeasurer.cpp"
	"src/dev/TrafficGenerator.cpp"
)

set_source_files_properties("${CMAKE_BINARY_DIR}/Version.cpp" PROPERTIES GENERATED 1)

add_executable(zbnt_daemon ${ZBNT_DAEMON_SRC})
add_dependencies(zbnt_daemon zbnt_genver)
target_link_libraries(zbnt_daemon Qt5::Core Qt5::Network ${libfdt} -lpthread)
target_include_directories(zbnt_daemon PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../common/include" "${CMAKE_BINARY_DIR}")

target_compile_definitions(
	zbnt_daemon PUBLIC
	ZBNT_CFG_PATH="${CFG_PATH}"
	ZBNT_FIRMWARE_PATH="${FIRMWARE_PATH}"
	ZBNT_SYSFS_PATH="${SYSFS_PATH}"
	ZBNT_CONFIGFS_PATH="${CONFIGFS_PATH}"
)

install(
	TARGETS zbnt_daemon
	PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
	DESTINATION bin
)
