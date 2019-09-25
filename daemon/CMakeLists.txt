cmake_minimum_required(VERSION 3.7)
project(zbnt_sw)

find_package(Qt5 COMPONENTS Core Network REQUIRED)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

set(ZBNT_SW_SRC
	"src/Main.cpp"
	"src/Utils.cpp"
	"src/WorkerThread.cpp"
	"src/BitstreamConfig.cpp"
	"src/MessageReceiver.cpp"
	"src/DiscoveryServer.cpp"
	"src/MeasurementServer.cpp"
)

add_executable(zbnt_daemon ${ZBNT_SW_SRC})
target_link_libraries(zbnt_daemon Qt5::Core Qt5::Network -lpthread)

install(
	TARGETS zbnt_daemon
	PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
	DESTINATION bin
)

install(
	FILES zbnt_daemon.service
	PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
	DESTINATION lib/systemd/system
)
