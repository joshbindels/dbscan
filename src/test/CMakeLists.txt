add_executable(test_dbscan test_dbscan.cpp)
target_link_libraries(test_dbscan PRIVATE gtest gtest_main)
target_include_directories(test_dbscan PRIVATE
    ${PROJECT_SOURCE_DIR}/common
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(test_dbscan PRIVATE
    BENCHMARK_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test_data/benchmark"
)

add_test(NAME test_dbscan COMMAND test_dbscan)

add_executable(test_dbscan_performance test_dbscan_performance.cpp)
target_include_directories(test_dbscan_performance PRIVATE
    ${PROJECT_SOURCE_DIR}/common
)
target_compile_definitions(test_dbscan_performance PRIVATE
    BENCHMARK_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test_data/benchmark"
)
