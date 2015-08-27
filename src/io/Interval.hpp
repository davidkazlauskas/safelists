
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

    enum class RelationResult {
        Equal,         // ====
        EmergesA,      // {..}
        EmergesB,      // .{}.
        OverlapsFront, // {.}.
        OverlapsBack,  // .{.}
        InFront,       // {}..
        InBack         // ..{}
    };

    Interval(int64_t start,int64_t end);
    Interval(const Interval&) = default;
    Interval(Interval&&) = default;
    Interval& operator=(const Interval&) = default;
    Interval& operator=(Interval&&) = default;

    Interval();

    int64_t start() const;
    int64_t end() const;
    bool isEmpty() const;

    // this interval is A, other is B
    RelationResult evaluate(const Interval& other) const;
    static RelationResult evaluate(const Interval& a,const Interval& b);

    bool operator==(const Interval& rhs) const;

    friend struct IntervalList;
private:
    int64_t _start;
    int64_t _end;
};

TEMPLATIOUS_BOILERPLATE_EXCEPTION(
    IntervalListIntervalDoesntBelongException,
    "Interval passed does not fit into the interval of this list."
);

struct IntervalList {

    IntervalList(const Interval& empty);

    IntervalList() = delete;
    IntervalList(IntervalList&&) = delete;
    IntervalList(const IntervalList&) = delete;

    typedef std::function<bool(const Interval&)> IntervalReceiveFunction;

    void traverseFilled(const IntervalReceiveFunction& func) const;
    void traverseEmpty(const IntervalReceiveFunction& func) const;
    // returns overlap, if any. empty range if none
    Interval append(const Interval& i);
    Interval closest(const Interval& i,Interval::RelationResult& outRel) const;

    friend struct IntervalListImpl;

#ifdef SAFELISTS_TESTING
    bool checkIntegrity() const;
#endif
private:
    Interval _emptyInterval;
    std::vector< Interval > _list;
};

}

#endif /* end of include guard: INTERVAL_C5VNKZTP */
