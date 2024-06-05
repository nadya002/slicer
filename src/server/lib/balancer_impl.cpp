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
#include <set>

namespace NSlicer {

// const int MinSlicePerNode = 50;
// const int MaxSlicePerNode = 200;
// const int MinRecomendSlicePerNode = 100;
// const int MaxRecomendSlicePerNode = 150;

const int MinSlicePerNode = 10;
const int MaxSlicePerNode = 50;
const int MinRecomendSlicePerNode = 20;
const int MaxRecomendSlicePerNode = 40;

// const int MinSlicePerNode = 2;
// const int MinRecomendSlicePerNode = 4;
// const int MaxRecomendSlicePerNode = 8;
// const int MaxSlicePerNode = 10;

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

BalancerImpl::BalancerImpl(const BalancerState& rangesToNode)
{
    spdlog::debug("BalancerImpl");
    for (auto& node : rangesToNode) {
        MappingRangesToNodes_[node.NodeId] = node.Ranges;
        for (auto& range : node.Ranges) {
            MappingStartIdToEndId_[range.Start] = range.End;
        }
    }
    // BalancingLogger_ = spdlog::basic_logger_mt("rebalance_logger", "logger/rebalancing_logger.txt");
    // BalancingLogger_->flush_on(spdlog::level::info);
}

void BalancerImpl::Initialize(const std::vector<std::string>& nodeIds)
{
    //BalancingLogger_->info("Initialize nodes");
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

void BalancerImpl::UpdateMetrics(const std::vector<TMetric>& metrics)
{
    //BalancingLogger_->info("UpdateMetrics");

    for (auto value : metrics) {
        if (MappingStartIdToEndId_.find(value.Range.Start) != MappingStartIdToEndId_.end() &&
            MappingStartIdToEndId_[value.Range.Start] == value.Range.End) {
                CurrentMappingStartIdToValue_[value.Range.Start] = value.Value_;
        } else {
            spdlog::error("Incorrect range in metrics ({}, {})", value.Range.Start, value.Range.End);
            throw std::invalid_argument("Unexpected range in metric");
        }
    }
}

bool BalancerImpl::CheckMetrics(const std::vector<TMetric>& metrics)
{
    for (auto value : metrics) {
        if (!(MappingStartIdToEndId_.find(value.Range.Start) != MappingStartIdToEndId_.end() &&
            MappingStartIdToEndId_[value.Range.Start] == value.Range.End)) {
            spdlog::warn("Incorrect range in metrics ({}, {})", value.Range.Start, value.Range.End);
            return false;
        }
    }
    return true;
}

BalancerDiff BalancerImpl::Rebalance(const std::vector<TMetric>& metrics)
{
    UpdateMetrics(metrics);
    MergeSlices();
    RebalanceRangesV2();
    SplitSlices();
    return GetMappingRangesToNodes();
}

std::vector<TRangesToNode> BalancerImpl::GetMappingRangesToNodes()
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

void BalancerImpl::RegisterNewNodes(const std::vector<std::string>& nodeIds)
{
    if (MappingRangesToNodes_.size() == 0) {
        Initialize(nodeIds);
    } else {
        if (nodeIds.size() > 0) {
            for (size_t nodeIndex = 0; nodeIndex < nodeIds.size(); ++nodeIndex) {
                std::list<TRange> ranges;
                MappingRangesToNodes_[nodeIds[nodeIndex]] = ranges;
            }
        }
    }
}

void BalancerImpl::UnregisterNode(const std::vector<std::string>& nodeIds)
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

std::list<TRange> BalancerImpl::SplitSpecificSlice(TRange range, int64_t n)
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
            end = currentStart + newSliceSize - 1;
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

std::vector<std::list<TRange>::iterator> BalancerImpl::ExtractAllRanges()
{
    std::vector<std::list<TRange>::iterator> result;
    for (auto& [nodeId, rangeList] : MappingRangesToNodes_) {
        for (auto it = rangeList.begin(); it != rangeList.end(); ++it) {
            result.push_back(it);
        }
    }
    return result;
}

void BalancerImpl::MergeSlices()
{
    spdlog::debug("Merge slices");
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

        if (cost > avr && CurrentMappingStartIdToValue_.size() <= minRecommendRangeCount) {
            break;
        }
        auto firstRangeIt = minCostRange.second.first;
        auto secondRangeIt = minCostRange.second.second;

        auto firstRange = *firstRangeIt;
        auto secondRange = *secondRangeIt;

        spdlog::debug(
            "Merge two ranges with load {}: ({}, {}) and ({}, {})",
            cost,
            firstRange.Start,
            firstRange.End,
            secondRange.Start,
            secondRange.End);

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

void BalancerImpl::SplitSlices()
{
    spdlog::debug("Spilt slices");
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
                auto oldRange = *rangeIter;
                auto newSlices = SplitSpecificSlice(*rangeIter, 2);
                rangeIter = rangeList.erase(rangeIter);
                spdlog::debug(
                    "Spliting hot range ({}, {}) to ranges ({}, {}) and ({}, {})",
                    oldRange.Start,
                    oldRange.End,
                    newSlices.begin()->Start,
                    newSlices.begin()->End,
                    newSlices.rbegin()->Start,
                    newSlices.rbegin()->End);
                rangeList.insert(rangeIter, newSlices.begin(), newSlices.end());

            } else {
                break;
            }
        }
        if (rangeList.size() < MinRecomendSlicePerNode) {
            if (rangeList.size() == 0) {
                break;
            }
            std::multimap<double, std::list<NSlicer::TRange>::const_iterator> valueToIter;
            for (auto it = rangeList.begin(); it != rangeList.end(); ++it) {
                valueToIter.insert({CurrentMappingStartIdToValue_[it->Start], it});
            }

            for (int i = 0; i < MinSlicePerNode - static_cast<int>(rangeList.size()); i++) {
                //std::cerr << "size of valueToIter is" << valueToIter.size();
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

void ApplyingDiffsToState(BalancerState* balanserState, const BalancerDiff& diff)
{
    *balanserState = diff;
}

double CountDisbalance(double a, double b)
{
    return 2 * std::max(a, b) / (a + b);
}

void BalancerImpl::RebalanceRanges()
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
        if (maxValue <= 0) {
            break;
        }
        spdlog::debug(
            "Realoc range ({}, {}) with disbalance {} from host {} to host {}",
            curRebalance.RangeIter->Start,
            curRebalance.RangeIter->End,
            maxValue,
            curRebalance.CurrentNode,
            curRebalance.NodeToMove);

        keyRealocCount += RangeSize(*curRebalance.RangeIter);
        auto rangeValue = CurrentMappingStartIdToValue_[curRebalance.RangeIter->Start];
        mappingNodeIdToValue[curRebalance.NodeToMove] += rangeValue;
        mappingNodeIdToValue[curRebalance.CurrentNode] -= rangeValue;
        MappingRangesToNodes_[curRebalance.NodeToMove].push_back(*curRebalance.RangeIter);
        MappingRangesToNodes_[curRebalance.CurrentNode].erase(curRebalance.RangeIter);
    }

}

class ModuloComparator {
public:
    // Конструктор с параметром - число, относительно которого считается модуль
    explicit ModuloComparator(int reference) : reference_(reference) {}

    // Оператор компаратора для set
    bool operator()(const std::pair<double, TRange>& a, const std::pair<double, TRange>& b) const {
        // Сравнение по абсолютной разнице чисел с reference_
        return a.first < b.first;
    }

private:
    int reference_;  // число, относительно которого сортируем
};

struct SetComparator {
    bool operator()(
        const std::pair<double, std::list<NSlicer::TRange>::iterator>& a,
        const std::pair<double, std::list<NSlicer::TRange>::iterator>& b) const {
            return a.first < b.first;
    }
};

void BalancerImpl::RebalanceRangesV2()
{
    std::multimap<double, BalanceInf> balancesCost;
    std::unordered_map<std::string, double> mappingNodeIdToValue;
    std::set<std::pair<double, std::string>> loadToNode;
    //TSortRangeComparator comp(&CurrentMappingStartIdToValue_);
    std::unordered_map<std::string, std::set<std::pair<double, std::list<NSlicer::TRange>::iterator>, SetComparator>> nodeToSetRanges;

    for (auto& [nodeId, rangeList] : MappingRangesToNodes_) {
        double value = 0;
        for (auto it = rangeList.begin(); it != rangeList.end(); ++it) {
            value += CurrentMappingStartIdToValue_[it->Start];
            nodeToSetRanges[nodeId].insert({CurrentMappingStartIdToValue_[it->Start], it});
        }
        mappingNodeIdToValue[nodeId] = value;
        loadToNode.insert({value, nodeId});
    }

    uint64_t keyRealocCount = 0;
    while (true) {
        if (keyRealocCount > MaxKeyReallocForRebalance) {
            break;
        }
        auto minLoadNode = *loadToNode.begin();
        auto maxLoadNode = *loadToNode.rbegin();

        auto maRealocLoad = (maxLoadNode.first - minLoadNode.first) / 2;
        auto maxNodeId = maxLoadNode.second;
        auto minNodeId = minLoadNode.second;

        auto it = nodeToSetRanges[maxNodeId].upper_bound({maRealocLoad, {}});

        if (it == nodeToSetRanges[maxNodeId].begin()) {
            break;
        } else {
            it--;
            if (it->first == 0) {
                break;
            }

            spdlog::debug(
                "Realoc range ({}, {}) with disbalance {} from host {} to host {}",
                (it->second)->Start,
                (it->second)->End,
                it->first,
                maxNodeId,
                minNodeId);

            keyRealocCount += RangeSize(*(it->second));
            loadToNode.erase(loadToNode.begin());
            loadToNode.erase(maxLoadNode);
            minLoadNode.first += it->first;
            loadToNode.insert(minLoadNode);
            maxLoadNode.first -= it->first;
            loadToNode.insert(maxLoadNode);

            auto newRange = MappingRangesToNodes_[minNodeId].insert(MappingRangesToNodes_[minNodeId].end(), *(it->second));

            MappingRangesToNodes_[maxNodeId].erase(it->second);

            auto newIt = *it;
            newIt.second = newRange;
            nodeToSetRanges[maxNodeId].erase(it);
            nodeToSetRanges[minNodeId].insert(newIt);
        }
    }

}

double BalancerImpl::ComputeAverage()
{
    spdlog::debug("ComputeAverage");
    double su = 0;
    double cnt = 0;
    for (const auto& value : CurrentMappingStartIdToValue_) {
        su += value.second;
        cnt++;
    }
    return cnt != 0 ? su / cnt : 0;
}

} // NSlicer