
add_executable(balancer_lib_test test_balancer_lib.cpp)

add_test(NAME balancer_lib_test COMMAND unit)

target_link_libraries(
        balancer_lib_test
        gtest_main
        Balancer
)
