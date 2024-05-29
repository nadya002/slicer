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

class Balancer
{
public:
    Balancer();
    ~Balancer();

    void Initialize(const std::vector<std::string>& nodeIds);

    void UpdateMetrics(const std::vector<TMetric>& metrics);

    std::vector<TRangesToNode> GetMappingRangesToNodes();

    void RegisterNewNodes(const std::vector<std::string>& nodeIds);

    void UnregisterNode(const std::vector<std::string>& nodeIds);

    void Rebalance();

private:
    class TSortRangeComparator {
    public:
        std::unordered_map<uint64_t, double>* CurrentMappingStartIdToValue_;

        TSortRangeComparator(std::unordered_map<uint64_t, double>* mappingStartIdToValue)
            : CurrentMappingStartIdToValue_(mappingStartIdToValue) {}

        bool operator()(const TRange& a, const TRange& b) {
            return (*CurrentMappingStartIdToValue_)[a.Start] > (*CurrentMappingStartIdToValue_)[b.Start];
        }
    };

    std::unordered_map<std::string, std::list<TRange>> MappingRangesToNodes_;
    std::unordered_map<uint64_t, double> CurrentMappingStartIdToValue_;
    std::unordered_map<uint64_t, double> LastMappingStartIdToValue_;

    std::unordered_map<uint64_t, uint64_t> MappingStartIdToEndId_;
    std::condition_variable Cv_;
    std::mutex Mutex_;
    bool NewMetrica_ = false;
    std::shared_ptr<spdlog::logger> BalancingLogger_;

    void SplitSlices();
    void RebalanceRanges();
    void MergeSlices();
    double ComputeAverage();
    std::vector<std::list<TRange>::iterator> ExtractAllRanges();
    std::list<TRange> SplitSpecificSlice(TRange range, int64_t n);
};

void RebalancingThread(Balancer* balancer);

} // NSlicer