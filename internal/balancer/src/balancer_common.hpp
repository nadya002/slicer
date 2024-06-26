#pragma once

#include <string>
#include <list>
#include <inttypes.h>

namespace balancer {
struct TRange {
    uint64_t Start;
    uint64_t End;
};

struct TMetric {
    TRange Range;
    double Value_;
};

struct TRangesToNode {
    std::string NodeId;
    std::list<TRange> Ranges;
};

struct TDiffsV2 {
    std::string NodeId;
    std::list<TRange> deletedRanges;
    std::list<TRange> newRanges;
};

using TDiffs = TRangesToNode;

}