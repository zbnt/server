
find_package(Qt5 COMPONENTS Core Network REQUIRED)
find_library(libfdt "libfdt.so")

set(ZBNT_SW_SRC
	"src/Main.cpp"
	"src/Utils.cpp"
	"src/Settings.cpp"
	"src/WorkerThread.cpp"
	"src/MessageReceiver.cpp"
	"src/DiscoveryServer.cpp"
	"src/BitstreamManager.cpp"
	"src/MeasurementServer.cpp"

	"src/dev/AbstractDevice.cpp"
	"src/dev/AxiDma.cpp"
	"src/dev/DmaBuffer.cpp"
	"src/dev/SimpleTimer.cpp"
	"src/dev/FrameDetector.cpp"
	"src/dev/StatsCollector.cpp"
	"src/dev/LatencyMeasurer.cpp"
	"src/dev/TrafficGenerator.cpp"
)

add_executable(zbnt_daemon ${ZBNT_SW_SRC})
target_link_libraries(zbnt_daemon Qt5::Core Qt5::Network ${libfdt} -lpthread)
target_include_directories(zbnt_daemon PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../include")

target_compile_definitions(
	zbnt_daemon PUBLIC
	ZBNT_VERSION="${PROJECT_VERSION}"
	ZBNT_VERSION_INT=${ZBNT_VERSION_INT}
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
