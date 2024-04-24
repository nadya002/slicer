#include <gtest/gtest.h>
#include "balancer.h"

TEST(MathFunctionsTest, AddTest) {
    std::vector<std::string> NodeIds;
    NodeIds.push_back("localhost");
    NSlicer::Balancer balancer;
    balancer.Initialize(NodeIds);
    auto res = balancer.GetMappingRangesToNodes();
    EXPECT_EQ(res.begin()->NodeId, "localhost");
    //EXPECT_EQ(3, add(1, 2));
}

