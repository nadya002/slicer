#include <vector>
#include <string>

namespace NSlicer {

struct TRange
{
    uint64_t Start;
    uint64_t End;
};

struct TMetric
{
    TRange Range;
    int64_t Value_;
};

struct TRangesToNode
{
    std::string NodeId;
    std::vector<TRange> Ranges;
};

} // NSlicer