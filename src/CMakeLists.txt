cmake_minimum_required(VERSION 3.15)

project(VehicleTracker
    VERSION 0.0.1
    DESCRIPTION "Track moving vehicles with Lidar"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(httplib INTERFACE)
target_include_directories(httplib INTERFACE vendored)

add_library(dbscan STATIC vendored/dbscan.cpp)
target_include_directories(dbscan PUBLIC vendored)

add_library(json INTERFACE)
target_include_directories(json INTERFACE vendored)

add_executable(service service.cpp)
target_link_libraries(service PRIVATE dbscan httplib json)

add_executable(processPointcloud processPointcloud.cpp)
target_link_libraries(processPointcloud PRIVATE httplib json)
