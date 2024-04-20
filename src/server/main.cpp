#include <balancer.h>

#include <iostream>

int main() {
    NSlicer::Balancer balancer;
    std::vector<std::string> NodeIds;
    NodeIds.push_back("localhost");
    balancer.Initialize(NodeIds);
    auto res = balancer.GetMappingRangesToNodes();
    std::cout << res.begin()->NodeId << std::endl;
    return 0;
}