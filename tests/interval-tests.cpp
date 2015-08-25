
#include <templatious/FullPack.hpp>
#include <io/Interval.hpp>

#include "catch.hpp"

TEMPLATIOUS_TRIPLET_STD;

TEST_CASE("interval_throw_wrong_interval","[interval]") {
    bool caught = false;
    try {
        SafeLists::Interval(2,1);
    } catch (SafeLists::IntervalStartGreaterThanEndException&) {
        caught = true;
    }

    REQUIRE( caught );
}

struct IntervalCollector {

    bool operator()(const SafeLists::Interval& i) {
        SA::add(_list,i);
        return true;
    }

    std::vector< SafeLists::Interval > _list;
};

TEST_CASE("interval_list_append","[interval]") {

    SafeLists::IntervalList list(0,1024);
    IntervalCollector colEmpty;
    IntervalCollector colFilled;

    list.append(16,32);
    list.traverseEmpty(colEmpty);
    list.traverseFilled(colFilled);

}