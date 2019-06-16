cmake_minimum_required(VERSION 3.7)
project(zbnt_sw)

find_package(Qt5 COMPONENTS Core Network REQUIRED)
find_library(gsl libgsl.so)
find_library(gslcblas libgslcblas.so)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")

set(ZBNT_SW_SRC
	"src/Main.cpp"
	"src/WorkerThread.cpp"
	"src/MeasurementServer.cpp"
)

add_executable(zbnt_daemon ${ZBNT_SW_SRC})
target_link_libraries(zbnt_daemon Qt5::Core Qt5::Network ${gsl} ${gslcblas} -lpthread)
