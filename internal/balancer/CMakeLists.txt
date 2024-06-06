cmake_minimum_required(VERSION 3.10)

find_package(unofficial-braft CONFIG REQUIRED)
find_package(unofficial-brpc CONFIG REQUIRED)

add_library(balancer_library
    SHARED
    src/node.cpp
    src/replicator.cpp
    src/balancer.cpp
    src/balancer_impl.cpp
)

target_include_directories(balancer_library PRIVATE include)

target_link_libraries(
    balancer_library
    
    unofficial::braft::braft-static
	unofficial::brpc::brpc-static
    spdlog
    nlohmann_json
    httplib
)