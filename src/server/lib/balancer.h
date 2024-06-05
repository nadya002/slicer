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
#include <functional>

namespace NSlicer {

class TBalancer;

//TBalancer* currentBalancer;

using CallbackFunc = std::function<void(BalancerDiff, std::function<void(bool, TBalancer*)>, TBalancer*)>;


struct TBalancerSnapshot
{
    BalancerState GetMapping();
    void Apply(const BalancerDiff& balancerDiff);

    BalancerState state;
};

class TBalancer
{
public:
    explicit TBalancer(
        const BalancerState& balancerState,
        const CallbackFunc& callback);

    ~TBalancer();

    void UpdateMetrics(const std::vector<TMetric>& metrics);

    BalancerDiff GetMappingRangesToNodes();

    void NotifyNodes(
        const std::vector<std::string>& newNodeIds,
        const std::vector<std::string>& deletedNodeIds);

    void ApplyDiffs(bool isSuccess);

private:
    BalancerImpl BalancerImpl_;

    BalancerDiff CurrentBalancerDiff_;
    TBalancerSnapshot Snapshot_;
    std::vector<TMetric> LastMetrics_;

    CallbackFunc CallbackFunc_;

    std::condition_variable Cv_;
    std::condition_variable SnapshotCv_;

    std::mutex BalancerImplMutex_;
    std::mutex SnapshotMutex_;
    std::mutex MetricsMutex_;

    std::thread RebalancingThread_;

    bool IsCallback_;
    bool NewMetrica_ = false;
    bool OnDestructor_ = false;
    bool IsReplicated = false;

    void RebalancingThreadFuncImpl();
    void TryToApplyDiffs(const BalancerDiff& diffs);

    static void RebalancingThreadFunc(TBalancer* balancer);
};

} // NSlicer