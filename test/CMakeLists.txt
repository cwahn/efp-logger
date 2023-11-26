add_executable(fmt_enum_test fmt_enum_test.cpp)
target_link_libraries(fmt_enum_test PRIVATE efp fmt)

add_executable(efp_log_test efp_log_test.cpp)
target_link_libraries(efp_log_test PRIVATE efp_log)

add_executable(efp_log_benchmark benchmark.cpp)
target_link_libraries(efp_log_benchmark PRIVATE efp_log)