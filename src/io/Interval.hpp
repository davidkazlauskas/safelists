
#ifndef INTERVAL_C5VNKZTP
#define INTERVAL_C5VNKZTP

#include <cstdint>
#include <vector>

namespace SafeLists {

struct Interval {

    Interval(int64_t start,int64_t end);

private:
    int64_t _start;
    int64_t _end;
};

struct IntervalList {

private:
    std::vector< Interval > _list;
};

}

#endif /* end of include guard: INTERVAL_C5VNKZTP */
