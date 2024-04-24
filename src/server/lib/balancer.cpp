#include "balancer.h"

#include <limits>

namespace NSlicer {

    void Balancer::Initialize(const std::vector<std::string>& nodeIds)
    {
        if (nodeIds.size() > 0) {
            int64_t nodeCount = nodeIds.size();
            uint64_t maxKey = std::numeric_limits<uint64_t>::max();
            uint64_t currentStart = 0;
            uint64_t rangeSize = (maxKey / nodeCount);

            for (int64_t nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
                uint64_t end;
                if (nodeIndex == nodeCount - 1 || currentStart > maxKey - rangeSize) {
                    end = maxKey;
                } else {
                    end = currentStart + rangeSize;
                }
                TRangesToNode rangesToNode;
                TRange range;
                range.Start = currentStart;
                range.End = end;
                rangesToNode.NodeId = nodeIds[nodeIndex];
                rangesToNode.Ranges.push_back(range);
                //  {
                //     .NodeId = nodeIds[nodeIndex],
                //     .Ranges = {TRange{
                //         .Start = currentStart,
                //         .End = end
                //     }}
                // };
                MappingRangesToNodes_.push_back(rangesToNode);
                currentStart = end + 1;
            }
        }
    }

    void Balancer::UpdateMetrics(const std::vector<TMetric>& metrics)
    {

    }

    std::list<TRangesToNode> Balancer::GetMappingRangesToNodes()
    {
        return MappingRangesToNodes_;
    }

    void Balancer::RegisterNewNode(std::string nodeId)
    {

    }

    void Balancer::UnregisterNode(std::string nodeId)
    {

    }

} // NSlicer