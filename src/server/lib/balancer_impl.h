#pragma once

#include "public.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"

#include <unordered_map>
#include <vector>
#include <list>
#include <string>
#include <condition_variable>

namespace NSlicer {

struct BalancerDiffV2
{
    int DiffId;
    std::vector<TDiffsV2> Diffs;
};

using BalancerState = std::vector<TRangesToNode>;
using BalancerDiff = std::vector<TDiffs>;

void ApplyingDiffsToState(BalancerState* balanserState, const BalancerDiff& diff);

class BalancerImpl
{
public:
    explicit BalancerImpl(const BalancerState& rangesToNode = {});

    bool CheckMetrics(const std::vector<TMetric>& metrics);

    void RegisterNewNodes(const std::vector<std::string>& nodeIds);

    void UnregisterNode(const std::vector<std::string>& nodeIds);

    BalancerDiff Rebalance(const std::vector<TMetric>& metrics);

    BalancerDiff GetMappingRangesToNodes();

private:
    class TSortRangeComparator {
    public:
        std::unordered_map<uint64_t, double>* CurrentMappingStartIdToValue_;

        TSortRangeComparator()
            {}

        explicit TSortRangeComparator(std::unordered_map<uint64_t, double>* mappingStartIdToValue)
            : CurrentMappingStartIdToValue_(mappingStartIdToValue) {}

        bool operator()(const TRange& a, const TRange& b) {
            return (*CurrentMappingStartIdToValue_)[a.Start] > (*CurrentMappingStartIdToValue_)[b.Start];
        }
    };

    std::unordered_map<std::string, std::list<TRange>> MappingRangesToNodes_;
    //std::unordered_map<std::string, std::list<TRange>> ResultMappingRangesToNodes_;

    std::unordered_map<uint64_t, double> CurrentMappingStartIdToValue_;
    //std::unordered_map<uint64_t, double> LastMappingStartIdToValue_;

    std::unordered_map<uint64_t, uint64_t> MappingStartIdToEndId_;

    std::shared_ptr<spdlog::logger> BalancingLogger_;

    void UpdateMetrics(const std::vector<TMetric>& metrics);

    void Initialize(const std::vector<std::string>& nodeIds);
    void SplitSlices();
    void RebalanceRanges();
    void MergeSlices();
    double ComputeAverage();
    void RebalanceRangesV2();

    // void RebalancingThreadFunc();
    std::vector<std::list<TRange>::iterator> ExtractAllRanges();
    std::list<TRange> SplitSpecificSlice(TRange range, int64_t n);
};

} // NSlicer