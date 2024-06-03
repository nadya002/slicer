#include <iostream>
#include <vector>
#include <thread>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "http.h"

int main() {
    spdlog::set_level(spdlog::level::debug);
    NSlicer::TBalancer Balancer_;
    NHttp::THttpSlicerServer server(&Balancer_);
    server.Start(NHttp::GetPort());
    return 0;
}
