#pragma once

#include <vector>
#include <string>
#include <list>
#include <cstdint>

namespace NSlicer {

struct TRange
{
    uint64_t Start;
    uint64_t End;
};

struct TMetric
{
    TRange Range;
    double Value_;
};

struct TRangesToNode
{
    std::string NodeId;
    std::list<TRange> Ranges;
};

using TDiffs = TRangesToNode;

} // NSlicer