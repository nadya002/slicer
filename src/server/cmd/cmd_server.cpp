#include <iostream>
#include <vector>
#include <thread>

#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "http.h"

int main() {
    NSlicer::Balancer Balancer_;

    std::thread rebalance(NSlicer::RebalancingThread, &Balancer_);
    NHttp::THttpSlicerServer server(&Balancer_);

    server.Start(NHttp::GetPort());
    rebalance.join();
    return 0;
}
