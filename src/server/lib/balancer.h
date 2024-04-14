#include <vector.h>

namespace NSlicer {

class Balancer
{
public:
    Balancer(int64_t NodeCount);

    void UpdateMetrics(const std::vector<TMetric>& metrics);

    std::vector<TRangesToNode> GetMappingRangesToNodes();

    int64_t RegisterNewNode();

    void UnregisterNode(int64_t NodeId);

private:

};

} // NSlicer