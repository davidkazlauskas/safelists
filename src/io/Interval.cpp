
#include <random>
#include <templatious/FullPack.hpp>

#include <util/Misc.hpp>

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

bool Interval::operator!=(const Interval& rhs) const {
    return !(*this == rhs);
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
        if (slider == 0) {
            slider = 1;
        }
        bool keepgoing = true;
        const Interval* previous = nullptr;
        const Interval* prevPrev = nullptr;
        for (;;) {
            if (demarcation < 0) {
                outRel = SA::getByIndex(list._list,0)
                    .evaluate(interval);
                return 0;
            } else if (demarcation >= size) {
                outRel = SA::getByIndex(list._list,size - 1)
                    .evaluate(interval);
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

    static bool isCorrupted(const IntervalList& list) {
        auto iter = SA::begin(list._list);
        auto end = SA::end(list._list);
        Interval victim;
        while (iter != end) {
            if (victim.end() >= iter->start()) {
                return true;
            }
            victim = *iter;
            ++iter;
        }

        return false;
    }

    static bool livesInside(const IntervalList& list,const Interval& i) {
        return list._emptyInterval.start() <= i.start()
            && list._emptyInterval.end() >= i.end();
    }

    static bool areEqual(const IntervalList& a,const IntervalList& b) {
        if (a._emptyInterval != b._emptyInterval) {
            return false;
        }

        if (SA::size(a._list) != SA::size(b._list)) {
            return false;
        }

        bool isGood = true;
        SM::traverse(
            [&](const Interval& left,const Interval& right) {
                isGood &= left == right;
                return isGood;
            },
            a._list,
            b._list
        );

        return isGood;
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

int64_t Interval::size() const {
    return _end - _start;
}

IntervalList::IntervalList(const Interval& empty)
    : _emptyInterval(empty)
{
}

IntervalList::IntervalList(IntervalList&& other) :
    _emptyInterval(other._emptyInterval),
    _list(std::move(other._list))
{
    other._emptyInterval = Interval();
    SA::clear(other._list);
}

IntervalList& IntervalList::operator=(IntervalList&& other) {
    _emptyInterval = other._emptyInterval;
    _list = std::move(other._list);
    return *this;
}

IntervalList IntervalList::clone() const {
    IntervalList target(_emptyInterval);
    target._list = _list;
    return target;
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

    if (!IntervalListImpl::livesInside(*this,i)) {
        throw IntervalListIntervalDoesntBelongException();
    }

    //if (IntervalListImpl::isCorrupted(*this)) {
        //int cholo = 7;
    //}

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
            if (iter->end() <= i.end())
            {
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
                if (iter->start() >= original->start()) {
                    *iter = nuller;
                } else {
                    if (iter->end() >= original->start()) {
                        *original = Interval(iter->start(),original->end());
                        *iter = nuller;
                    }
                    break;
                }
                --iter;
            }
        }

        if (original != iter && iter == beg
            && iter->end() >= original->start())
        {
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
        // doesn't matter, interval swallows already
        return Interval();
    } else if (r == RR::EmergesB) {
        Interval nuller;
        auto iter = SA::iterAt(_list,res);
        auto beg = SA::begin(_list);
        auto end = SA::end(_list);
        auto starter = iter;

        *iter = i;

        // forward purge
        ++iter;
        for (; iter != end; ++iter) {
            if (iter->start() > starter->end()) {
                break;
            } else if (iter->start() <= starter->end()) {
                if (iter->end() > starter->end()) {
                    *starter = Interval(starter->start(),iter->end());
                    *iter = nuller;
                    break;
                } else {
                    *iter = nuller;
                }
            }
        }

        iter = starter;
        // backward purge
        if (iter != beg) {
            --iter;
            for (; iter != beg; --iter) {
                if (iter->end() < starter->start()) {
                    break;
                } else if (iter->start() >= starter->start()) {
                    *iter = nuller;
                } else if (iter->end() >= starter->start()) {
                    *starter = Interval(iter->start(),starter->end());
                    *iter = nuller;
                    break;
                }
            }

            if (iter == beg) {
                if (iter->end() >= starter->start()) {
                    if (iter->start() < starter->start()) {
                        *starter = Interval(iter->start(),starter->end());
                    }
                    *iter = nuller;
                }
            }
        }

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

bool IntervalList::doesBelong(const Interval& i) const {
    return IntervalListImpl::livesInside(*this,i);
}

bool IntervalList::isFilled() const {
    if (SA::size(_list) != 1) {
        return false;
    }
    return this->_emptyInterval == SA::getByIndex(_list,0);
}

namespace {

struct IntervalCollector {

    std::function<bool(const SafeLists::Interval&)> f() {
        return [=](const SafeLists::Interval& i) {
            SA::add(this->_list,i);
            return true;
        };
    }

    std::vector< SafeLists::Interval > _list;

};

}

void IntervalList::randomEmptyIntervals(int64_t size,const IntervalReceiveFunction& func) const {
    int64_t sizeShipped = 0;
    std::mt19937 generator(size);
    IntervalCollector collector;
    this->traverseEmpty(collector.f());
    auto& list = collector._list;

    auto end = SA::end(list);
    while (!this->isFilled() && sizeShipped < size) {
        int64_t remaining = size - sizeShipped;
        auto point = generator() % SA::size(list);
        auto picked = SA::iterAt(list,point);
        while (picked->isEmpty() && picked != end) {
            ++picked;
        }
        if (picked != end) {
            auto& current = *picked;
            auto currentSize = current.size();

            if (currentSize > remaining) {
                auto splitPoint = generator() % (currentSize - remaining);
                if (splitPoint > 0 && splitPoint != currentSize - remaining) {
                    auto currentStart = current.start() + splitPoint;
                    Interval toShip(currentStart,currentStart + remaining);
                    Interval cutOff(toShip.end(),current.end());
                    current._end = toShip.start();
                    SA::add(list,cutOff);
                    bool funcResult = func(toShip);
                    if (!funcResult) {
                        return;
                    }
                    sizeShipped += remaining;
                } else if (splitPoint == currentSize - remaining) {
                    auto currentStart = current.start() + splitPoint;
                    Interval toShip(currentStart,current.end());
                    auto shipSize = toShip.size();
                    bool funcResult = func(toShip);
                    if (!funcResult) {
                        return;
                    }
                    current._end = currentStart;
                    sizeShipped += shipSize;
                } else if (splitPoint == 0) {
                    auto currentStart = current.start();
                    Interval toShip(currentStart,currentStart + currentSize);
                    current._start = toShip.end();
                    bool funcResult = func(toShip);
                    if (!funcResult) {
                        return;
                    }
                    sizeShipped += currentSize;
                }
            } else if (currentSize <= remaining) {
                auto toShip = current;
                bool funcResult = func(toShip);
                if (!funcResult) {
                    return;
                }
                sizeShipped += currentSize;
                current = Interval();
            }
        }
    }
}

#ifdef SAFELISTS_TESTING
bool IntervalList::checkIntegrity() const {
    return !IntervalListImpl::isCorrupted(*this);
}
#endif

int64_t IntervalList::nonEmptyIntervalCount() const {
    return SA::size(_list);
}

Interval IntervalList::range() const {
    return _emptyInterval;
}

bool IntervalList::isDefined() const {
    return !_emptyInterval.isEmpty();
}

void writeIntervalList(const IntervalList& list,std::ostream& output) {
    const int SZ = sizeof(int64_t);
    char buf[SZ];
    auto count = list.nonEmptyIntervalCount();
    auto range = list.range();

    auto numToStream =
        [&](const int64_t& num) {
            writeI64AsLittleEndian(num,buf);
            output.write(buf,sizeof(num));
        };

    SM::callEach(
        numToStream,
        count,
        range.start(),
        range.end());

    list.traverseFilled(
        [&](const Interval& i) {
            SM::callEach(
                numToStream,
                i.start(),
                i.end());
            return true;
        }
    );
}

IntervalList readIntervalList(std::istream& input) {
    const int SZ = sizeof(int64_t);
    char buf[SZ];
    int64_t num;

    auto numFromStream =
        [&]() {
            input.read(buf,sizeof(buf));
            readI64FromLittleEndian(num,buf);
            return num;
        };

    auto count = numFromStream();
    auto rangeStart = numFromStream();
    auto rangeEnd = numFromStream();

    IntervalList result(Interval(rangeStart,rangeEnd));

    TEMPLATIOUS_REPEAT( count ) {
        rangeStart = numFromStream();
        rangeEnd = numFromStream();
        result.append(Interval(rangeStart,rangeEnd));
    }

    return result;
}

bool areIntervalListsEqual(const IntervalList& a,const IntervalList& b) {
    return IntervalListImpl::areEqual(a,b);
}

}

