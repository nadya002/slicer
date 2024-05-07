#include "balancer.h"

#include <cmath>
#include <limits>
#include <unordered_map>
#include <map>
#include <list>
#include <iostream>
#include <algorithm>

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

uint64_t RangeSize(const TRange& range)
{
    if (range.Start == 0 && range.End == std::numeric_limits<uint64_t>::max()) {
        return std::numeric_limits<uint64_t>::max();
    } else {
        return range.End - range.Start + 1;
    }
}
void Balancer::Initialize(const std::vector<std::string>& nodeIds)
{
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
    for (auto value : metrics) {
        if (MappingStartIdToEndId_.find(value.Range.Start) != MappingStartIdToEndId_.end() &&
            MappingStartIdToEndId_[value.Range.Start] == value.Range.End) {
                MappingStartIdToValue_[value.Range.Start] = value.Value_;
                std::cerr << "metr " << value.Range.Start << " " << value.Value_ << std::endl;
        } else {
            throw std::invalid_argument("Unexpected range in metric");
        }
    }
    MergeSlices();
    SplitSlices();
}

std::vector<TRangesToNode> Balancer::GetMappingRangesToNodes()
{
    std::vector<TRangesToNode> result;
    for (const auto& value : MappingRangesToNodes_) {
        result.push_back(TRangesToNode{
            .NodeId = value.first,
            .Ranges = value.second
        });
    }
    return result;
}

void Balancer::RegisterNewNode(std::string nodeId)
{

}

void Balancer::UnregisterNode(std::string nodeId)
{

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

    if (MappingStartIdToValue_.find(range.Start) != MappingStartIdToValue_.end()) {
        rangeValue = MappingStartIdToValue_[range.Start];
        MappingStartIdToValue_.erase(range.Start);
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
        MappingStartIdToValue_[currentStart] = rangeValue;
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
        double mergeCost = MappingStartIdToValue_[firstRange->Start] +  MappingStartIdToValue_[secondRange->Start];
        mergeCostsToRange[{mergeCost, firstRange->Start}] =
            {firstRange, secondRange};
        nextRange[firstRange->Start] = secondRange;
    }
    size_t minRangeCount = MinSlicePerNode * MappingRangesToNodes_.size();
    size_t minRecommendRangeCount = MinSlicePerNode * MappingRangesToNodes_.size();

    uint64_t keyRealocCount = 0;
    while (MappingStartIdToValue_.size() > minRangeCount) {
        if (keyRealocCount >= MaxKeyReallocForMerge) {
            break;
        }
        if (mergeCostsToRange.size() == 0) {
            break;
        }
        auto minCostRange = *(mergeCostsToRange.begin());
        auto cost = minCostRange.first.first;
        std::cerr << "costs " << cost << std::endl;
        if (cost > avr && MappingStartIdToValue_.size() <= minRecommendRangeCount) {
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
        MappingStartIdToValue_.erase(secondRange.Start);
        MappingStartIdToValue_[firstRange.Start] = minCostRange.first.first;

        mergeCostsToRange.erase(mergeCostsToRange.begin());
        if (nextRange.find(secondRange.Start) != nextRange.end()) {
            nextRange[firstRange.Start] = nextRange[secondRange.Start];
            nextRange.erase(secondRange.Start);
            auto newCost = cost + MappingStartIdToValue_[nextRange[firstRange.Start]->Start];
            mergeCostsToRange[{newCost, firstRange.Start}] = {newFirstIt, nextRange[firstRange.Start]};
        }
    }

}
void Balancer::SplitSlices()
{
    auto avr = ComputeAverage();
    for (auto& [nodeId, rangeList] : MappingRangesToNodes_) {
        //std::map<uint64_t, uint64_t> HotSlicesKoef;
        rangeList.sort(TSortRangeComparator(&MappingStartIdToValue_));
        std::list<TRange>::iterator rangeIter = rangeList.begin();
        int rangeCount = rangeList.size();

        while (rangeIter != rangeList.end()) {
            if (rangeCount > MaxSlicePerNode) {
                break;
            }
            if (MappingStartIdToValue_[rangeIter->Start] > 2 * avr && RangeSize(*rangeIter) > 1) {
                rangeCount++;
                auto newSlices = SplitSpecificSlice(*rangeIter, 2);
                rangeIter = rangeList.erase(rangeIter);
                rangeList.insert(rangeIter, newSlices.begin(), newSlices.end());

            } else {
                rangeIter++;
            }
        }
        if (rangeList.size() < MinRecomendSlicePerNode) {
            std::map<double, std::list<NSlicer::TRange>::const_iterator> valueToIter;
            for (auto it = rangeList.begin(); it != rangeList.end(); ++it) {
                valueToIter[MappingStartIdToValue_[it->Start]] = it;
            }

            for (int i = 0; i < MinSlicePerNode - static_cast<int>(rangeList.size()); i++) {
                auto hotRange = valueToIter.cbegin();
                auto range = *(hotRange->second);
                if (RangeSize(range) > 1) {
                    auto newSlices = SplitSpecificSlice(range, 2);
                    auto newIter = rangeList.erase(hotRange->second);
                    auto newElems = rangeList.insert(newIter, newSlices.begin(), newSlices.end());
                    valueToIter[MappingStartIdToValue_[newElems->Start]] = newElems;
                    newElems++;
                    valueToIter[MappingStartIdToValue_[newElems->Start]] = newElems;
                    valueToIter.erase(hotRange);
                }
            }
        }

    }


}

double Balancer::ComputeAverage()
{
    double su = 0;
    double cnt = 0;
    for (const auto& value : MappingStartIdToValue_) {
        su += value.second;
        cnt++;
    }
    return cnt != 0 ? su / cnt : 0;
}

} // NSlicer