
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
    int64_t size() const;

    // this interval is A, other is B
    RelationResult evaluate(const Interval& other) const;
    static RelationResult evaluate(const Interval& a,const Interval& b);

    bool operator==(const Interval& rhs) const;
    bool operator!=(const Interval& rhs) const;

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
    IntervalList(IntervalList&&);
    IntervalList(const IntervalList&) = delete;
    IntervalList& operator=(IntervalList&&);
    IntervalList& operator=(const IntervalList&) = delete;

    typedef std::function<bool(const Interval&)> IntervalReceiveFunction;

    void traverseFilled(const IntervalReceiveFunction& func) const;
    void traverseEmpty(const IntervalReceiveFunction& func) const;
    // returns overlap, if any. empty range if none
    Interval append(const Interval& i);
    Interval closest(const Interval& i,Interval::RelationResult& outRel) const;
    void randomEmptyIntervals(int64_t size,const IntervalReceiveFunction& func) const;

    bool doesBelong(const Interval& i) const;
    bool isFilled() const;
    bool isEmpty() const;

    friend struct IntervalListImpl;

    IntervalList clone() const;

    int64_t nonEmptyIntervalCount() const;
    Interval range() const;

    bool isDefined() const;

#ifdef SAFELISTS_TESTING
    bool checkIntegrity() const;
#endif
private:
    Interval _emptyInterval;
    std::vector< Interval > _list;
};

// streams should be binary, oh ye who looks for trouble
void writeIntervalList(const IntervalList& list,std::ostream& output);
IntervalList readIntervalList(std::istream& input);
bool areIntervalListsEqual(const IntervalList& a,const IntervalList& b);

}

#endif /* end of include guard: INTERVAL_C5VNKZTP */
