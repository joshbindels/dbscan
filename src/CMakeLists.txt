cmake_minimum_required(VERSION 3.15)

project(VehicleTracker
    VERSION 0.0.1
    DESCRIPTION "Track moving vehicles with Lidar"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

find_package(prometheus-cpp CONFIG QUIET)
if (NOT TARGET prometheus-cpp::core)
  set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  set(ENABLE_PUSH OFF CACHE BOOL "" FORCE)
  set(ENABLE_PULL ON CACHE BOOL "" FORCE)
  FetchContent_Declare(
    prometheus-cpp
    GIT_REPOSITORY https://github.com/jupp0r/prometheus-cpp.git
    GIT_TAG v1.3.0
    GIT_SHALLOW TRUE
    GIT_SUBMODULES "3rdparty/civetweb"
    GIT_SUBMODULES_RECURSE TRUE
  )
  FetchContent_MakeAvailable(prometheus-cpp)
endif()

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_library(httplib INTERFACE)
target_include_directories(httplib INTERFACE libs)

add_library(json INTERFACE)
target_include_directories(json INTERFACE libs)

add_executable(service service/service.cpp)
target_link_libraries(service PRIVATE
    httplib
    json
    prometheus-cpp::pull
)
target_include_directories(service PRIVATE common service)

add_executable(processPointcloud client/processPointcloud.cpp)
target_link_libraries(processPointcloud PRIVATE httplib json)
target_include_directories(processPointcloud PRIVATE common client)

add_executable(test_dbscan test/test_dbscan.cpp)
target_link_libraries(test_dbscan PRIVATE gtest gtest_main)
target_include_directories(test_dbscan PRIVATE common test libs)
target_compile_definitions(test_dbscan PRIVATE
    BENCHMARK_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/test_data/benchmark"
)

add_test(NAME test_dbscan COMMAND test_dbscan)

add_executable(test_dbscan_performance test/test_dbscan_performance.cpp)
target_include_directories(test_dbscan_performance PRIVATE common)
target_compile_definitions(test_dbscan_performance PRIVATE
    BENCHMARK_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test/test_data/benchmark"
)
