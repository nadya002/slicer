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

namespace NSlicer {

// const int MinSlicePerNode = 50;
// const int MaxSlicePerNode = 200;
// const int MinRecomendSlicePerNode = 100;
// const int MaxRecomendSlicePerNode = 150;

const int MinSlicePerNode = 2;
const int MinRecomendSlicePerNode = 4;
const int MaxRecomendSlicePerNode = 8;
const int MaxSlicePerNode = 10;

const uint64_t MaxKeyReallocForMerge = std::numeric_limits<uint64_t>::max() / 100;
const uint64_t MaxKeyReallocForRebalance = 9 * (std::numeric_limits<uint64_t>::max() / 100);

uint64_t RangeSize(const TRange& range)
{
    if (range.Start == 0 && range.End == std::numeric_limits<uint64_t>::max()) {
        return std::numeric_limits<uint64_t>::max();
    } else {
        return range.End - range.Start + 1;
    }
}

Balancer::Balancer()
{
    BalancingLogger_ = spdlog::basic_logger_mt("rebalance_logger", "logger/rebalancing_logger.txt");
    BalancingLogger_->flush_on(spdlog::level::info);
}

Balancer::~Balancer()
{
    BalancingLogger_->flush();

}

void Balancer::Initialize(const std::vector<std::string>& nodeIds)
{
    BalancingLogger_->info("Initialize nodes");
    if (nodeIds.size() > 0) {
        int64_t nodeCount = nodeIds.size();
        auto slices = SplitSpecificSlice(TRange{
            .Start = 0,
            .End = std::numeric_limits<uint64_t>::max()
        }, nodeCount);
        auto currentSlice = slices.begin();

        for (int64_t nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
            auto ranges = SplitSpecificSlice(*currentSlice, MinRecomendSlicePerNode);
            currentSlice++;
            MappingRangesToNodes_[nodeIds[nodeIndex]] = ranges;
        }
    }
}

void Balancer::UpdateMetrics(const std::vector<TMetric>& metrics)
{
    BalancingLogger_->info("UpdateMetrics");
    {
        std::lock_guard<std::mutex> lk(Mutex_);
        for (auto value : metrics) {
            if (MappingStartIdToEndId_.find(value.Range.Start) != MappingStartIdToEndId_.end() &&
                MappingStartIdToEndId_[value.Range.Start] == value.Range.End) {
                    LastMappingStartIdToValue_[value.Range.Start] = value.Value_;
            } else {
                throw std::invalid_argument("Unexpected range in metric");
            }
        }
        BalancingLogger_->info("Finish UpdateMetrics");
        NewMetrica_ = true;
    }
    std::cerr << "notify_one" << std::endl;
    Cv_.notify_one();
}

void Balancer::Rebalance()
{
    BalancingLogger_->info("Wait for metrica");
    std::unique_lock<std::mutex> lk(Mutex_);

    while (!NewMetrica_) {
        BalancingLogger_->info("wait");
        Cv_.wait(lk, [this]{ return NewMetrica_; });
    }
    CurrentMappingStartIdToValue_ = LastMappingStartIdToValue_;
    NewMetrica_ = false;
    lk.unlock();
    Cv_.notify_one();

    BalancingLogger_->info("Start new iteration of rebalancing");
    std::cerr << "star balancing" << std::endl;
    MergeSlices();
    RebalanceRanges();
    SplitSlices();

}

std::vector<TRangesToNode> Balancer::GetMappingRangesToNodes()
{
    std::vector<TRangesToNode> result;
    for (const auto& [nodeId, ranges] : MappingRangesToNodes_) {
        result.push_back(TRangesToNode{
            .NodeId = nodeId,
            .Ranges = ranges
        });
    }
    return result;
}

void Balancer::RegisterNewNodes(const std::vector<std::string>& nodeIds)
{
    if (nodeIds.size() > 0) {
        for (size_t nodeIndex = 0; nodeIndex < nodeIds.size(); ++nodeIndex) {
            std::list<TRange> ranges;
            MappingRangesToNodes_[nodeIds[nodeIndex]] = ranges;
        }
    }
}

void Balancer::UnregisterNode(const std::vector<std::string>& nodeIds)
{
    std::unordered_map<std::string, double> mappingNodeIdToValue;
    std::multimap<double, std::string> valueNode;
    std::unordered_set<std::string> nodesSet;
    for (auto& node : nodeIds) {
        nodesSet.insert(node);
    }

    for (auto& [nodeId, ranges] : MappingRangesToNodes_) {
        if (nodesSet.find(nodeId) == nodesSet.end()) {
            double value = 0;
            for (auto& range : ranges) {
                value += CurrentMappingStartIdToValue_[range.Start];
            }
            mappingNodeIdToValue[nodeId] = value;
            valueNode.insert({value, nodeId});
        }
    }

    for (auto& nodeId : nodeIds) {
        for (auto& range : MappingRangesToNodes_[nodeId]) {
            auto minNode = valueNode.begin();
            auto nodeTo = minNode->second;
            MappingRangesToNodes_[nodeTo].push_back(range);
            mappingNodeIdToValue[nodeTo] += CurrentMappingStartIdToValue_[range.Start];
            valueNode.erase(valueNode.begin());
            valueNode.insert({mappingNodeIdToValue[nodeTo], nodeTo});
        }
        MappingRangesToNodes_.erase(nodeId);
    }

}

std::list<TRange> Balancer::SplitSpecificSlice(TRange range, int64_t n)
{
    uint64_t sliceSize = (range.End - range.Start);
    if (sliceSize < std::numeric_limits<uint64_t>::max()) {
        sliceSize++;
    }
    uint64_t newSliceSize = sliceSize / n;
    std::list<TRange> result;
    uint64_t currentStart = range.Start;

    double rangeValue = 1;

    if (CurrentMappingStartIdToValue_.find(range.Start) != CurrentMappingStartIdToValue_.end()) {
        rangeValue = CurrentMappingStartIdToValue_[range.Start];
        CurrentMappingStartIdToValue_.erase(range.Start);
    }
    rangeValue = rangeValue / n;

    if (MappingStartIdToEndId_.find(range.Start) != MappingStartIdToEndId_.end()) {
        MappingStartIdToEndId_.erase(range.Start);
    }

    for (int i = 0; i < n; i++) {
        uint64_t end;
        if (i == n - 1) {
            end = range.End;
        } else {
            end = currentStart + newSliceSize;
        }
        CurrentMappingStartIdToValue_[currentStart] = rangeValue;
        result.push_back(TRange{
            .Start = currentStart,
            .End = end
        });
        MappingStartIdToEndId_[currentStart] = end;
        currentStart = end + 1;
    }
    return result;
}

std::vector<std::list<TRange>::iterator> Balancer::ExtractAllRanges()
{
    std::vector<std::list<TRange>::iterator> result;
    for (auto& [nodeId, rangeList] : MappingRangesToNodes_) {
        for (auto it = rangeList.begin(); it != rangeList.end(); ++it) {
            result.push_back(it);
        }
    }
    return result;
}

void Balancer::MergeSlices()
{
    auto avr = ComputeAverage();
    auto ranges = ExtractAllRanges();
    std::sort(ranges.begin(), ranges.end(), [](const std::list<TRange>::iterator& a, const std::list<TRange>::iterator& b) {
        return a->Start < b->Start;
    });

    std::map<std::pair<double, uint64_t>, std::pair<std::list<TRange>::iterator, std::list<TRange>::iterator>> mergeCostsToRange;
    std::unordered_map<uint64_t, std::string> startToNodes;

    for (auto& [nodeId, rangeList] : MappingRangesToNodes_) {
        for (auto it = rangeList.begin(); it != rangeList.end(); ++it) {
            startToNodes[it->Start] = nodeId;
        }
    }

    std::unordered_map<uint64_t, std::list<TRange>::iterator> nextRange;
    for (size_t i = 0; i + 1 < ranges.size(); i++) {
        auto firstRange = ranges[i];
        auto secondRange = ranges[i + 1];
        double mergeCost = CurrentMappingStartIdToValue_[firstRange->Start] +  CurrentMappingStartIdToValue_[secondRange->Start];
        mergeCostsToRange[{mergeCost, firstRange->Start}] =
            {firstRange, secondRange};
        nextRange[firstRange->Start] = secondRange;
    }
    size_t minRangeCount = MinSlicePerNode * MappingRangesToNodes_.size();
    size_t minRecommendRangeCount = MinSlicePerNode * MappingRangesToNodes_.size();

    uint64_t keyRealocCount = 0;
    while (CurrentMappingStartIdToValue_.size() > minRangeCount) {
        if (keyRealocCount >= MaxKeyReallocForMerge) {
            break;
        }
        if (mergeCostsToRange.size() == 0) {
            break;
        }
        auto minCostRange = *(mergeCostsToRange.begin());
        auto cost = minCostRange.first.first;
        std::cerr << "costs " << cost << std::endl;
        if (cost > avr && CurrentMappingStartIdToValue_.size() <= minRecommendRangeCount) {
            break;
        }
        auto firstRangeIt = minCostRange.second.first;
        auto secondRangeIt = minCostRange.second.second;

        auto firstRange = *firstRangeIt;
        auto secondRange = *secondRangeIt;
        keyRealocCount += RangeSize(secondRange);

        // Updating MappingRangesToNodes_
        MappingRangesToNodes_[startToNodes[firstRange.Start]].erase(firstRangeIt);
        MappingRangesToNodes_[startToNodes[secondRange.Start]].erase(secondRangeIt);

        firstRange.End = secondRange.End;
        auto newFirstIt = MappingRangesToNodes_[startToNodes[firstRange.Start]].insert(MappingRangesToNodes_[startToNodes[firstRange.Start]].begin(), firstRange);

        // Updating MappingStartIdToEndId_
        MappingStartIdToEndId_.erase(secondRange.Start);
        MappingStartIdToEndId_[firstRange.Start] = firstRange.End;

        // Updating MappingStartIdToValue_
        CurrentMappingStartIdToValue_.erase(secondRange.Start);
        CurrentMappingStartIdToValue_[firstRange.Start] = minCostRange.first.first;

        mergeCostsToRange.erase(mergeCostsToRange.begin());
        if (nextRange.find(secondRange.Start) != nextRange.end()) {
            nextRange[firstRange.Start] = nextRange[secondRange.Start];
            nextRange.erase(secondRange.Start);
            auto newCost = cost + CurrentMappingStartIdToValue_[nextRange[firstRange.Start]->Start];
            mergeCostsToRange[{newCost, firstRange.Start}] = {newFirstIt, nextRange[firstRange.Start]};
        }
    }

}

void Balancer::SplitSlices()
{
    auto avr = ComputeAverage();
    for (auto& [nodeId, rangeList] : MappingRangesToNodes_) {
        rangeList.sort(TSortRangeComparator(&CurrentMappingStartIdToValue_));
        std::list<TRange>::iterator rangeIter = rangeList.begin();
        int rangeCount = rangeList.size();

        while (rangeIter != rangeList.end()) {
            if (rangeCount > MaxSlicePerNode) {
                break;
            }
            if (CurrentMappingStartIdToValue_[rangeIter->Start] > 2 * avr && RangeSize(*rangeIter) > 1) {
                rangeCount++;
                auto newSlices = SplitSpecificSlice(*rangeIter, 2);
                rangeIter = rangeList.erase(rangeIter);
                rangeList.insert(rangeIter, newSlices.begin(), newSlices.end());

            } else {
                rangeIter++;
            }
        }
        if (rangeList.size() < MinRecomendSlicePerNode) {
            std::multimap<double, std::list<NSlicer::TRange>::const_iterator> valueToIter;
            for (auto it = rangeList.begin(); it != rangeList.end(); ++it) {
                valueToIter.insert({CurrentMappingStartIdToValue_[it->Start], it});
            }

            for (int i = 0; i < MinSlicePerNode - static_cast<int>(rangeList.size()); i++) {
                auto hotRange = valueToIter.cbegin();
                auto range = *(hotRange->second);
                if (RangeSize(range) > 1) {
                    auto newSlices = SplitSpecificSlice(range, 2);
                    auto newIter = rangeList.erase(hotRange->second);
                    auto newElems = rangeList.insert(newIter, newSlices.begin(), newSlices.end());
                    valueToIter.insert({CurrentMappingStartIdToValue_[newElems->Start], newElems});
                    newElems++;
                    valueToIter.insert({CurrentMappingStartIdToValue_[newElems->Start], newElems});
                    valueToIter.erase(hotRange);
                }
            }
        }

    }


}

struct BalanceInf
{
    std::list<NSlicer::TRange>::iterator RangeIter;
    std::string NodeToMove;
    std::string CurrentNode;
};

double CountDisbalance(double a, double b)
{
    return 2 * std::max(a, b) / (a + b);
}

void Balancer::RebalanceRanges()
{
    std::multimap<double, BalanceInf> balancesCost;
    std::unordered_map<std::string, double> mappingNodeIdToValue;

    for (auto& ranges : MappingRangesToNodes_) {
        double value = 0;
        for (auto& range : ranges.second) {
            value += CurrentMappingStartIdToValue_[range.Start];
        }
        mappingNodeIdToValue[ranges.first] = value;
    }

    uint64_t keyRealocCount = 0;
    while (true) {
        if (keyRealocCount > MaxKeyReallocForRebalance) {
            break;
        }
        double maxValue = -1;
        BalanceInf curRebalance;
        for (auto& [nodeId, rangeList] : MappingRangesToNodes_) {
             for (auto it = rangeList.begin(); it != rangeList.end(); ++it) {
                for (auto& nodeTo : mappingNodeIdToValue) {
                    if (nodeTo.first != nodeId) {
                        double firstNodeVal = mappingNodeIdToValue[nodeId];
                        double secondNodeVal = nodeTo.second;
                        double curDisbalance = CountDisbalance(firstNodeVal, secondNodeVal);
                        double newFirstNodeVal = firstNodeVal - CurrentMappingStartIdToValue_[it->Start];
                        double newSecondNodeVal = secondNodeVal + CurrentMappingStartIdToValue_[it->Start];
                        double newDisbalanceCost = CountDisbalance(newFirstNodeVal, newSecondNodeVal);
                        double finalValue = (curDisbalance - newDisbalanceCost) / log(RangeSize(*it) + 2);
                        if (finalValue > maxValue) {
                            maxValue = finalValue;
                            curRebalance.RangeIter = it;
                            curRebalance.NodeToMove = nodeTo.first;
                            curRebalance.CurrentNode = nodeId;
                        }
                    }
                }
            }
        }
        if (maxValue < 0) {
            break;
        }
        keyRealocCount += RangeSize(*curRebalance.RangeIter);
        auto rangeValue = CurrentMappingStartIdToValue_[curRebalance.RangeIter->Start];
        mappingNodeIdToValue[curRebalance.NodeToMove] += rangeValue;
        mappingNodeIdToValue[curRebalance.CurrentNode] -= rangeValue;
        MappingRangesToNodes_[curRebalance.NodeToMove].push_back(*curRebalance.RangeIter);
        MappingRangesToNodes_[curRebalance.CurrentNode].erase(curRebalance.RangeIter);
    }

}

double Balancer::ComputeAverage()
{
    double su = 0;
    double cnt = 0;
    for (const auto& value : CurrentMappingStartIdToValue_) {
        su += value.second;
        cnt++;
    }
    return cnt != 0 ? su / cnt : 0;
}

void RebalancingThread(Balancer* balancer) {
    while(true) {
        balancer->Rebalance();
    }
}

} // NSlicer