#include "balancer.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

#include <cmath>
#include <limits>
#include <unordered_map>
#include <map>
#include <list>
#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <unordered_set>
#include <optional>

namespace NSlicer {

BalancerState TBalancerSnapshot::GetMapping()
{
    return state;
}

void TBalancerSnapshot::Apply(const BalancerDiff& balancerDiff)
{
    ApplyingDiffsToState(&state, balancerDiff);
}


TBalancer::TBalancer(
    const BalancerState& balancerState,
    bool isCallback,
    const CallbackFunc& callback)
    : BalancerImpl_(balancerState)
    , IsCallback_(isCallback)
    , CallbackFunc_(callback)
    , RebalancingThread_(RebalancingThreadFunc, this)
{ }

TBalancer::~TBalancer()
{
    {
        std::lock_guard<std::mutex> lk(BalancerImplMutex_);
        OnDestructor_ = true;
    }
    Cv_.notify_all();
    RebalancingThread_.join();
}

void TBalancer::NotifyNodes(
    const std::vector<std::string>& newNodeIds,
    const std::vector<std::string>& deletedNodeIds)
{
    BalancerDiff diffs;
    {
        std::lock_guard<std::mutex> lockBalancerMutex(BalancerImplMutex_);
        BalancerImpl_.RegisterNewNodes(newNodeIds);
        BalancerImpl_.UnregisterNode(deletedNodeIds);
        diffs = BalancerImpl_.GetMappingRangesToNodes();
    }

    {
        std::lock_guard<std::mutex> lockSnapshotMutex(SnapshotMutex_);
        if (IsCallback_) {
            CallbackFunc_(diffs);
        }
        Snapshot_.Apply(diffs);
    }

}

void TBalancer::UpdateMetrics(const std::vector<TMetric>& metrics)
{
    spdlog::debug("UpdateMetrics");
    {
        std::lock_guard<std::mutex> lk(MetricsMutex_);
        LastMetrics_ = metrics;
        NewMetrica_ = true;
    }
    Cv_.notify_one();
}

void TBalancer::RebalancingThreadFunc(TBalancer* balancer)
{
    balancer->RebalancingThreadFuncImpl();
}

void TBalancer::RebalancingThreadFuncImpl()
{
    while (true) {
        std::unique_lock<std::mutex> lk(MetricsMutex_);

        while (!NewMetrica_ && !OnDestructor_) {
            spdlog::debug("Waiting fo new metrics ...");
            Cv_.wait(lk, [this]{ return NewMetrica_ || OnDestructor_;});
        }
        if (OnDestructor_) {
            break;
        }
        NewMetrica_ = false;
        lk.unlock();

        BalancerDiff diffs;
        {
            std::lock_guard<std::mutex> lockBalancerMutex(BalancerImplMutex_);
            if (!BalancerImpl_.CheckMetrics(LastMetrics_)) {
                continue;
            }
            diffs = BalancerImpl_.Rebalance(LastMetrics_);
        }

        {
            std::lock_guard<std::mutex> lockBalancerMutex(SnapshotMutex_);
            if (IsCallback_) {
                CallbackFunc_(diffs);
            }
            Snapshot_.Apply(diffs);
        }

        spdlog::debug("Finish rebalacing iteration");
    }
}

BalancerDiff TBalancer::GetMappingRangesToNodes()
{
    {
        std::lock_guard<std::mutex> lockBalancerMutex(SnapshotMutex_);
        //std::cerr << "GetMappingRangesToNodes " << Snapshot_.GetMapping().size() << std::endl;
        return Snapshot_.GetMapping();
    }
}

} // NSlicer