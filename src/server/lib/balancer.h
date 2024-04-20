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
    Balancer();

    void Initialize(const std::vector<std::string>& nodeIds);

    void UpdateMetrics(const std::vector<TMetric>& metrics);

    std::list<TRangesToNode> GetMappingRangesToNodes();

    void RegisterNewNode(std::string nodeId);

    void UnregisterNode(std::string nodeId);

private:
    std::list<TRangesToNode> MappingRangesToNodes_;
    //std::vector<TRange>
};

} // NSlicer