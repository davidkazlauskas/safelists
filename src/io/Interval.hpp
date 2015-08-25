
#ifndef INTERVAL_C5VNKZTP
#define INTERVAL_C5VNKZTP

#include <cstdint>
#include <vector>

namespace SafeLists {

struct Interval {

    Interval(int64_t start,int64_t end);

    int64_t start() const;
    int64_t end() const;
    bool isEmpty() const;

private:
    int64_t _start;
    int64_t _end;
};

struct IntervalList {

    IntervalList(int64_t emptyStart,int64_t emptyEnd);

    IntervalList() = delete;
    IntervalList(IntervalList&&) = delete;
    IntervalList(const IntervalList&) = delete;

private:
    Interval _emptyInterval;
    std::vector< Interval > _list;
};

}

#endif /* end of include guard: INTERVAL_C5VNKZTP */
