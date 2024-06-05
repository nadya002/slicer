cmake_minimum_required(VERSION 3.10)

add_executable(balancer_cli main.cpp)

include_directories(../internal/balancer/include)

target_link_libraries(
		balancer_cli
		balancer_library
		spdlog
)