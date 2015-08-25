
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

TEST_CASE("interval_evaluation","[interval]") {
    typedef SafeLists::Interval Int;
    Int::RelationResult r;

    r = Int::evaluate(Int(0,1),Int(2,3));
    REQUIRE( r == Int::RelationResult::InFront );

    r = Int::evaluate(Int(0,1),Int(1,3));
    REQUIRE( r == Int::RelationResult::OverlapsFront );

    r = Int::evaluate(Int(0,1),Int(-2,-1));
    REQUIRE( r == Int::RelationResult::InBack );

    r = Int::evaluate(Int(0,1),Int(-2,0));
    REQUIRE( r == Int::RelationResult::OverlapsBack );

    r = Int::evaluate(Int(0,1),Int(0,2));
    REQUIRE( r == Int::RelationResult::OverlapsFront );

    r = Int::evaluate(Int(0,1),Int(-2,1));
    REQUIRE( r == Int::RelationResult::OverlapsBack );

    r = Int::evaluate(Int(0,1),Int(0,1));
    REQUIRE( r == Int::RelationResult::Equal );

    r = Int::evaluate(Int(-1,2),Int(0,1));
    REQUIRE( r == Int::RelationResult::EmergesA );

    r = Int::evaluate(Int(0,1),Int(-1,2));
    REQUIRE( r == Int::RelationResult::EmergesB );
}

struct IntervalCollector {

    bool operator()(const SafeLists::Interval& i) {
        SA::add(_list,i);
        return true;
    }

    std::vector< SafeLists::Interval > _list;
};

TEST_CASE("interval_list_append","[interval]") {

    typedef SafeLists::Interval Int;

    SafeLists::IntervalList list(Int(0,1024));
    IntervalCollector colEmpty;
    IntervalCollector colFilled;

    list.append(Int(16,32));
    list.traverseEmpty(colEmpty);
    list.traverseFilled(colFilled);

}
