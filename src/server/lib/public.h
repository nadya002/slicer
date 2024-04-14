
namespace NSlicer {

struct TRange
{
    int64_t start, end;
}

struct TMetric
{
    TRange Range;
    int64_t Value_;
}

struct TRangesToNode
{
    int64_t NodeId;
    std::vector<TRange> Ranges;
}

} // NSlicer