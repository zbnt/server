
find_package(Qt5 COMPONENTS Core Network REQUIRED)

set(ZBNT_SW_SRC
	"src/Main.cpp"
	"src/Utils.cpp"
	"src/Settings.cpp"
	"src/WorkerThread.cpp"
	"src/BitstreamConfig.cpp"
	"src/MessageReceiver.cpp"
	"src/DiscoveryServer.cpp"
	"src/MeasurementServer.cpp"
)

add_executable(zbnt_daemon ${ZBNT_SW_SRC})
target_link_libraries(zbnt_daemon Qt5::Core Qt5::Network -lpthread)
target_include_directories(zbnt_daemon PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../include")
target_compile_definitions(zbnt_daemon PUBLIC ZBNT_DAEMON_CFG_PATH="/etc/zbnt")

install(
	TARGETS zbnt_daemon
	PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ
	DESTINATION bin
)
