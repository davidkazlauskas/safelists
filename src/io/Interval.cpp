
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
            if (demarcation < 0) {
                return 0;
            } else if (demarcation >= size) {
                return size - 1;
            }

            const Interval* current = &SA::getByIndex(vec,demarcation);
            RelationResult r = current->evaluate(interval);
            if (r == RelationResult::InFront) {
                if (current == previous || prevPrev == current) {
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
    typedef Interval::RelationResult RR;
    RR r;
    auto res = IntervalListImpl::findClosest(*this,i,r);
    if (res == -1) {
        SA::add(_list,i);
        return Interval();
    }

    if (r == RR::Equal) {
        return i;
    } else if (r == RR::InFront) {
        if (SA::size(_list) - 1 == res) {
            SA::add(_list,i);
        } else {
            SA::insert(_list,
                SA::iterAt(_list,res + 1),
                i);
        }
    } else if (r == RR::InBack) {
        if (res == 0) {
            SA::insert(_list,SA::begin(_list),i);
        } else {
            SA::insert(_list,SA::iterAt(_list,res),i);
        }
    } else if (r == RR::OverlapsFront) {
        Interval nuller;
        auto iter = SA::iterAt(_list,res);
        auto original = iter;
        auto end = SA::end(_list);
        *iter = Interval(iter->start(),i.end());
        ++iter;
        while (iter != end) {
            if (iter->end() >= i.end()) {
                *iter = nuller;
            } else {
                break;
            }
            ++iter;
        }

        if (iter != end && iter->start() <= i.end()) {
            *original = Interval(original->start(),iter->end());
            *iter = nuller;
        }

        SA::clear(
            SF::filter(
                _list,
                [](const Interval& i) {
                    return i.isEmpty();
                }
            )
        );
    } else if (r == RR::OverlapsBack) {
        // I'm so lazy, this need to be rewritten
        Interval nuller;
        auto iter = SA::iterAt(_list,res);
        auto original = iter;
        auto beg = SA::begin(_list);
        *iter = Interval(i.start(),iter->end());
        if (res > 0) {
            --iter;
            while (iter != beg) {
                if (iter->end() >= i.end()) {
                    *iter = nuller;
                } else {
                    break;
                }
            }
        }

        if (iter == beg && iter->end() >= original->start()) {
            *original = Interval(iter->start(),original->end());
            *iter = nuller;
        }

        SA::clear(
            SF::filter(
                _list,
                [](const Interval& i) {
                    return i.isEmpty();
                }
            )
        );
    } else if (r == RR::EmergesA) {
        assert( false && "Not yet implemented." );
    } else if (r == RR::EmergesB) {
        Interval nuller;
        auto iter = SA::iterAt(_list,res);

        *iter = i;

        SA::clear(
            SF::filter(
                _list,
                [](const Interval& i) {
                    return i.isEmpty();
                }
            )
        );
    }

    return Interval();
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

