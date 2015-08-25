
#include "Interval.hpp"

namespace SafeLists {

Interval::Interval(int64_t start,int64_t end)
 : _start(start), _end(end) {}

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

}

