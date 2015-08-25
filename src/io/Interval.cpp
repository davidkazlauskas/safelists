
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

    static int64_t findClosest(
            const IntervalList& list,
            const Interval& interval,
            RelationResult& outRel)
    {
        auto size = SA::size(list._list);
        if (0 == size) {
            return -1;
        }

        auto& vec = list._list;
        auto demarcation = size / 2;
        auto slider = demarcation / 2;
        bool keepgoing = true;
        const Interval* previous = nullptr;
        const Interval* prevPrev = nullptr;
        for (;;) {
            if (demarcation < 0 || demarcation >= size) {
                return -1;
            }

            const Interval* current = &SA::getByIndex(vec,demarcation);
            RelationResult r = current->evaluate(interval);
            if (r == RelationResult::InFront) {
                if (prevPrev == current) {
                    outRel = r;
                    return demarcation;
                }
                demarcation += slider;
            } else if (r == RelationResult::InBack) {
                if (prevPrev == current) {
                    outRel = r;
                    return demarcation;
                }
                demarcation -= slider;
            } else if (
                r == RelationResult::EmergesA
                || r == RelationResult::EmergesB
                || r == RelationResult::OverlapsBack
                || r == RelationResult::OverlapsFront
                || r == RelationResult::Equal
            )
            {
                outRel = r;
                return demarcation;
            }

            slider /= 2;
            if (slider <= 0) {
                slider = 1;
            }

            prevPrev = previous;
            previous = current;
        }
    }
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

Interval IntervalList::closest(const Interval& i,Interval::RelationResult& outRel) const {
    auto res = IntervalListImpl::findClosest(*this,i,outRel);
    if (res > 0) {
        return SA::getByIndex(_list,res);
    }
    Interval outNull;
    return outNull;
}

}

