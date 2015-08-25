
#include <templatious/FullPack.hpp>
#include "Interval.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace SafeLists {

Interval::Interval(int64_t start,int64_t end) :
    _start(start), _end(end)
{
    if (_start > _end)
        throw IntervalStartGreaterThanEndException();
}

// -1/-1 -> unused inteval
Interval::Interval() :
    _start(-1), _end(-1) {}

int64_t Interval::start() const {
    return _start;
}

int64_t Interval::end() const {
    return _end;
}

bool Interval::isEmpty() const {
    return _start == _end;
}

bool Interval::operator==(const Interval& rhs) const {
    return _start == rhs._start && _end == rhs._end;
}

struct IntervalListImpl {

    typedef Interval::RelationResult RelationResult;

    static RelationResult evaluate(const Interval& a,const Interval& b) {
        if (a.end() < b.start()) {
            return RelationResult::InFront;
        }

        if (b.end() < a.start()) {
            return RelationResult::InBack;
        }

        if (a == b) {
            return RelationResult::Equal;
        }

        if (a.start() < b.start() && a.end() > b.end()) {
            return RelationResult::EmergesA;
        }

        if (b.start() < a.start() && b.end() > a.end()) {
            return RelationResult::EmergesB;
        }

        if (a.start() <= b.start() && a.end() <= b.end()) {
            return RelationResult::OverlapsFront;
        }

        if (b.start() <= a.start() && b.end() <= a.end()) {
            return RelationResult::OverlapsBack;
        }

        assert( false && "Didn't expect this milky." );
    }

    //static int64_t findClosest(const IntervalList& list) {

    //}
};

auto Interval::evaluate(const Interval& other) const
    -> RelationResult
{
    return IntervalListImpl::evaluate(*this,other);
}

auto Interval::evaluate(const Interval& a,const Interval& b)
    -> RelationResult
{
    return a.evaluate(b);
}

IntervalList::IntervalList(const Interval& empty)
    : _emptyInterval(empty)
{
}

void IntervalList::traverseFilled(const IntervalReceiveFunction& func) const {
    if (SA::size(_list) == 0) {
        return;
    }
    Interval victim;
    TEMPLATIOUS_FOREACH(auto& i,_list) {
        victim = i;
        func(victim);
    }
}

void IntervalList::traverseEmpty(const IntervalReceiveFunction& func) const {
    Interval victim;
    if (SA::size(_list) == 0) {
        victim = _emptyInterval;
        func(victim);
        return;
    }

    auto beg = SA::begin(_list);
    auto end = SA::end(_list);
    if (beg->start() > 0) {
        func(Interval(0,beg->start()));
    }

    victim = *beg;
    ++beg;
    while (beg != end) {
        func(Interval(victim.end(),beg->start()));
        victim = *beg;
        ++beg;
    }

    if (victim.end() < _emptyInterval.end()) {
        func(Interval(victim.end(),_emptyInterval.end()));
    }
}

// returns overlap, if any. empty range if none
Interval IntervalList::append(const Interval& i) {

}

}

