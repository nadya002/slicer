#pragma once

#include "public.h"

#include <unordered_map>
#include <vector>
#include <list>
#include <string>

namespace NSlicer {

class Balancer
{
public:
    void Initialize(const std::vector<std::string>& nodeIds);

    void UpdateMetrics(const std::vector<TMetric>& metrics);

    std::vector<TRangesToNode> GetMappingRangesToNodes();

    void RegisterNewNode(std::string nodeId);

    void UnregisterNode(std::string nodeId);

private:
    class TSortRangeComparator {
    public:
        std::unordered_map<uint64_t, double>* MappingStartIdToValue_;

        TSortRangeComparator(std::unordered_map<uint64_t, double>* mappingStartIdToValue)
            : MappingStartIdToValue_(mappingStartIdToValue) {}

        bool operator()(const TRange& a, const TRange& b) {
            return (*MappingStartIdToValue_)[a.Start] > (*MappingStartIdToValue_)[b.Start];
        }
    };

    std::unordered_map<std::string, std::list<TRange>> MappingRangesToNodes_;
    std::unordered_map<uint64_t, double> MappingStartIdToValue_;
    std::unordered_map<uint64_t, uint64_t> MappingStartIdToEndId_;

    void SplitSlices();
    void MergeSlices();
    double ComputeAverage();
    std::vector<std::list<TRange>::iterator> ExtractAllRanges();
    std::list<TRange> SplitSpecificSlice(TRange range, int64_t n);
};

} // NSlicer