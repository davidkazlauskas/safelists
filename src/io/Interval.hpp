
#ifndef INTERVAL_C5VNKZTP
#define INTERVAL_C5VNKZTP

#include <cstdint>
#include <vector>
#include <functional>

#include <templatious/util/Exceptions.hpp>

namespace SafeLists {

TEMPLATIOUS_BOILERPLATE_EXCEPTION(
    IntervalStartGreaterThanEndException,
    "Beginning of interval cannot be greater than the end."
);

struct Interval {

    Interval(int64_t start,int64_t end);
    Interval(const Interval&) = default;
    Interval(Interval&&) = delete;
    Interval& operator=(const Interval&) = default;
    Interval& operator=(Interval&&) = delete;

    Interval();

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

    typedef std::function<bool(const Interval&)> IntervalReceiveFunction;

    void traverseFilled(const IntervalReceiveFunction& func);
    void traverseEmpty(const IntervalReceiveFunction& func);

private:
    Interval _emptyInterval;
    std::vector< Interval > _list;
};

}

#endif /* end of include guard: INTERVAL_C5VNKZTP */
