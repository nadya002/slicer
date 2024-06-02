#pragma once

#include "public.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "balancer_impl.h"
#include <unordered_map>
#include <vector>
#include <list>
#include <string>
#include <condition_variable>

namespace NSlicer {

struct TBalancerSnapshot
{
    BalancerState GetMapping();
    void Apply(const BalancerDiff& balancerDiff);

    BalancerState state;
};

class TBalancer
{
public:
    explicit TBalancer(const BalancerState& balancerState = {});

    ~TBalancer();

    void UpdateMetrics(const std::vector<TMetric>& metrics);

    BalancerDiff GetMappingRangesToNodes();

    void NotifyNodes(
        const std::vector<std::string>& newNodeIds,
        const std::vector<std::string>& deletedNodeIds);

private:
    BalancerImpl BalancerImpl_;
    TBalancerSnapshot Snapshot_;
    std::vector<TMetric> LastMetrics_;

    std::condition_variable Cv_;

    std::mutex BalancerImplMutex_;
    std::mutex SnapshotMutex_;
    std::mutex MetricsMutex_;

    std::thread RebalancingThread_;

    bool NewMetrica_ = false;
    bool OnDestructor_ = false;

    void RebalancingThreadFuncImpl();

    static void RebalancingThreadFunc(TBalancer* balancer);
};

} // NSlicer