
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

IntervalList::IntervalList(int64_t emptyStart,int64_t emptyEnd)
    : _emptyInterval(emptyStart,emptyEnd)
{
}

void IntervalList::traverseFilled(const IntervalReceiveFunction& func) {
    if (SA::size(_list) == 0) {
        return;
    }
    Interval victim;
    TEMPLATIOUS_FOREACH(auto& i,_list) {
        victim = i;
        func(victim);
    }
}

void IntervalList::traverseEmpty(const IntervalReceiveFunction& func) {
    Interval victim;
    if (SA::size(_list) == 0) {
        victim = _emptyInterval;
        func(victim);
        return;
    }

    auto beg = SA::begin(_list);

}

}

