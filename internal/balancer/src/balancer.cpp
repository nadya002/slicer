#include "balancer.hpp"
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

namespace balancer {

BalancerState TBalancerSnapshot::GetMapping() {
    return state;
}

void TBalancerSnapshot::Apply(const BalancerDiff &balancerDiff) {
    ApplyingDiffsToState(&state, balancerDiff);
}


TBalancer::TBalancer(
    const BalancerState &balancerState,
    const CallbackFunc &callback)
    : BalancerImpl_(balancerState), CallbackFunc_(callback), RebalancingThread_(RebalancingThreadFunc, this) {}

void TBalancer::ApplyDiffs(bool isSuccess) {
    spdlog::debug("apply diffs");
    {
        //std::lock_guard<std::mutex> lockSnapshotMutex(SnapshotMutex_);
        //spdlog::debug("lockSnapshotMutex");
        if (isSuccess) {
            spdlog::debug("success apply diffs");
            Snapshot_.Apply(CurrentBalancerDiff_);
        }
    }
    IsReplicated = true;
    SnapshotCv_.notify_one();
}

TBalancer::~TBalancer() {
    {
        std::lock_guard<std::mutex> lk(BalancerImplMutex_);
        OnDestructor_ = true;
    }
    Cv_.notify_all();
    RebalancingThread_.join();
}

void TBalancer::TryToApplyDiffs(const BalancerDiff &diffs) {
    spdlog::debug("try to apply diffs");
    std::unique_lock<std::mutex> lockSnapshotMutex(SnapshotMutex_);
    IsReplicated = false;
    CurrentBalancerDiff_ = diffs;
    auto applyingFunc = [&](bool isSuccess) {
        ApplyDiffs(isSuccess);
    };

    CallbackFunc_(diffs, applyingFunc);

    while (!IsReplicated) {
        SnapshotCv_.wait(lockSnapshotMutex, [this] { return IsReplicated; });
    }

    lockSnapshotMutex.unlock();
}

void TBalancer::NotifyNodes(
    const std::vector<std::string> &newNodeIds,
    const std::vector<std::string> &deletedNodeIds) {
    spdlog::debug("Notify nodes");
    BalancerDiff diffs;
    {
        std::lock_guard<std::mutex> lockBalancerMutex(BalancerImplMutex_);
        //spdlog::debug("lockBalancerMutex");
        BalancerImpl balancerImpl(Snapshot_.GetMapping());
        //spdlog::debug("balancerImpl");
        balancerImpl.RegisterNewNodes(newNodeIds);
        balancerImpl.UnregisterNode(deletedNodeIds);
        diffs = balancerImpl.GetMappingRangesToNodes();

        //spdlog::debug("TryToApplyDiffs");
        TryToApplyDiffs(diffs);
        //spdlog::debug("TryToApplyDiffs finish");
    }
}

void TBalancer::UpdateMetrics(const std::vector<TMetric> &metrics) {
    spdlog::debug("UpdateMetrics");
    {
        std::lock_guard<std::mutex> lk(MetricsMutex_);
        LastMetrics_ = metrics;
        NewMetrica_ = true;
    }
    Cv_.notify_one();
}

void TBalancer::RebalancingThreadFunc(TBalancer *balancer) {
    balancer->RebalancingThreadFuncImpl();
}

void TBalancer::RebalancingThreadFuncImpl() {
    while (true) {
        std::unique_lock<std::mutex> lk(MetricsMutex_);

        while (!NewMetrica_ && !OnDestructor_) {
            spdlog::debug("Waiting for new metrics ...");
            Cv_.wait(lk, [this] { return NewMetrica_ || OnDestructor_; });
        }
        if (OnDestructor_) {
            break;
        }
        NewMetrica_ = false;
        lk.unlock();

        BalancerDiff diffs;
        {
            std::lock_guard<std::mutex> lockBalancerMutex(BalancerImplMutex_);

            BalancerImpl balancerImpl(Snapshot_.GetMapping());
            if (!balancerImpl.CheckMetrics(LastMetrics_)) {
                continue;
            }
            diffs = balancerImpl.Rebalance(LastMetrics_);
            TryToApplyDiffs(diffs);
        }

        spdlog::debug("Finish rebalacing iteration");
    }
}

BalancerDiff TBalancer::GetMappingRangesToNodes() {
    {
        std::lock_guard<std::mutex> lockBalancerMutex(SnapshotMutex_);
        //std::cerr << "GetMappingRangesToNodes " << Snapshot_.GetMapping().size() << std::endl;
        return Snapshot_.GetMapping();
    }
}

} // NSlicer