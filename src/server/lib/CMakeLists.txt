
cmake_minimum_required(VERSION 3.10)

add_library(Balancer balancer.cpp)

target_include_directories(Balancer INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
	Balancer
    spdlog
)