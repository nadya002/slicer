#include <iostream>
#include <vector>
#include <thread>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "http.h"

void simpleCallback(NSlicer::BalancerDiff diff,  std::function<void(bool, NSlicer::TBalancer*)> func, NSlicer::TBalancer* balancer)
{
    func(true, balancer);
}

int main() {
    spdlog::set_level(spdlog::level::debug);
    NSlicer::TBalancer Balancer_({}, simpleCallback);
    NHttp::THttpSlicerServer server(&Balancer_);
    server.Start(NHttp::GetPort());
    return 0;
}
