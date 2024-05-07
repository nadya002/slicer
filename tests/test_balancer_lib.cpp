#include <gtest/gtest.h>

#include "public.h"
#include "balancer.h"

using namespace NSlicer;

std::vector<TRange> ExtractAllNodes(const std::vector<TRangesToNode>& ranges)
{
    std::vector<TRange> res;
    for (auto& value : ranges) {
        for (auto& range : value.Ranges) {
            res.push_back(range);
        }
    }
    return res;
}

bool CheckAllRanges(const std::vector<TRangesToNode>& ranges)
{
    auto res = ExtractAllNodes(ranges);
    std::sort(res.begin(), res.end(), [](const TRange& a, const TRange& b) {
        return a.Start < b.Start;
    });
    if (res.size() == 0) {
        return false;
    }

    uint64_t pred = 0;
    for (auto& value : res) {
        if (pred != value.Start) {
            return false;
        }
        pred = value.End + 1;
    }

    if (res.back().End != std::numeric_limits<uint64_t>::max()) {
         return false;
    }
    return true;
}

void prettyPrint(const std::vector<TRangesToNode>& ranges)
{
    for (auto range : ranges) {
        std::cerr << range.NodeId << std::endl;
        for (auto val : range.Ranges) {
            std::cerr << val.Start << " " << val.End << std::endl;
        }
    }
}

// TEST(InitializeTest, SimpleTest) {
//     std::vector<std::string> NodeIds;
//     NodeIds.push_back("localhost");
//     NSlicer::Balancer balancer;
//     balancer.Initialize(NodeIds);
//     auto res = balancer.GetMappingRangesToNodes();
//     EXPECT_EQ(res.begin()->NodeId, "localhost");
//     EXPECT_TRUE(CheckAllRanges(res));
// }

// TEST(InitializeTest, SimpleTest2) {
//     std::vector<std::string> NodeIds;
//     NodeIds.push_back("localhost");
//     NodeIds.push_back("172.12.31.32");
//     NSlicer::Balancer balancer;
//     balancer.Initialize(NodeIds);
//     auto res = balancer.GetMappingRangesToNodes();
//     EXPECT_TRUE(res.begin()->Ranges.size() > 50 && res.begin()->Ranges.size() < 200);
//     EXPECT_TRUE(CheckAllRanges(res));
// }

// TEST(InitializeTest, SeveralNodes) {
//     std::vector<std::string> NodeIds;
//     for (int i = 0; i < 20; i++) {
//         NodeIds.push_back(std::to_string(i));
//     }
//     NSlicer::Balancer balancer;
//     balancer.Initialize(NodeIds);
//     auto res = balancer.GetMappingRangesToNodes();
//     EXPECT_TRUE(res.begin()->Ranges.size() > 50 && res.begin()->Ranges.size() < 200);
//     EXPECT_TRUE(CheckAllRanges(res));
// }

// TEST(BalancerTest, SeveralNodes) {
//     std::vector<std::string> NodeIds;
//     for (int i = 0; i < 2; i++) {
//         NodeIds.push_back(std::to_string(i));
//     }
//     NSlicer::Balancer balancer;
//     balancer.Initialize(NodeIds);
//     auto res = balancer.GetMappingRangesToNodes();
//     EXPECT_TRUE(res.begin()->Ranges.size() > 50 && res.begin()->Ranges.size() < 200);
//     EXPECT_TRUE(CheckAllRanges(res));
//     auto ranges = ExtractAllNodes(res);
//     std::vector<TMetric> metrics;
//     int i = 0;
//     for (auto& val : ranges) {
//         double price = 0;
//         if (i < 5) {
//             price = 100;
//         }
//         i++;
//         metrics.push_back(TMetric{
//             .Range = val,
//             .Value_ = price
//         });
//     }
//     balancer.UpdateMetrics(metrics);
//     auto resAfterBalance = balancer.GetMappingRangesToNodes();
//     std::cerr << resAfterBalance[0].Ranges.size() << std::endl;
//     std::cerr << resAfterBalance[1].Ranges.size() << std::endl;
// }

TEST(MergeTest, SeveralNodes) {
    std::vector<std::string> NodeIds;
    for (int i = 0; i < 2; i++) {
        NodeIds.push_back(std::to_string(i));
    }
    NSlicer::Balancer balancer;
    balancer.Initialize(NodeIds);
    auto res = balancer.GetMappingRangesToNodes();
    std::cerr << "before balance" << std::endl;
    prettyPrint(res);
    std::cerr << std::endl;
    //EXPECT_TRUE(res.begin()->Ranges.size() > 50 && res.begin()->Ranges.size() < 200);
    //EXPECT_TRUE(CheckAllRanges(res));
    auto ranges = ExtractAllNodes(res);
    std::vector<TMetric> metrics;
    int i = 0;
    for (auto& val : ranges) {
        //std::cerr << val.Start << " " << val.End << " ";
        double price = 0;
        if (i < 2) {
            price = 100;
        }
        i++;
        metrics.push_back(TMetric{
            .Range = val,
            .Value_ = price
        });
    }
    //std::cerr << std::endl;
    balancer.UpdateMetrics(metrics);
    auto resAfterBalance = balancer.GetMappingRangesToNodes();

    std::cerr << "after balance" << std::endl;
    prettyPrint(resAfterBalance);
    std::cerr << std::endl;
    // std::cerr << resAfterBalance[0].Ranges.size() << std::endl;
    // std::cerr << resAfterBalance[1].Ranges.size() << std::endl;
}

