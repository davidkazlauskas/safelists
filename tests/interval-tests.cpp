
#include <random>

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

    std::function<bool(const SafeLists::Interval&)> f() {
        return [=](const SafeLists::Interval& i) {
            SA::add(this->_list,i);
            return true;
        };
    }

    std::vector< SafeLists::Interval > _list;
};

TEST_CASE("interval_list_append","[interval]") {

    typedef SafeLists::Interval Int;

    SafeLists::IntervalList list(Int(0,1024));
    IntervalCollector colEmpty;
    IntervalCollector colFilled;
    auto &eList = colEmpty._list;
    auto &fList = colFilled._list;

    list.append(Int(16,32));
    list.traverseEmpty(colEmpty.f());
    list.traverseFilled(colFilled.f());

    REQUIRE( eList[0] == Int(0,16) );
    REQUIRE( eList[1] == Int(32,1024) );
    REQUIRE( SA::size( eList ) == 2 );

    REQUIRE( fList[0] == Int(16,32) );
    REQUIRE( SA::size( fList ) == 1 );
}

TEST_CASE("interval_list_append_shorten","[interval]") {
    typedef SafeLists::Interval Int;

    SafeLists::IntervalList list(Int(0,1024));
    IntervalCollector colEmpty;
    IntervalCollector colFilled;
    auto &eList = colEmpty._list;
    auto &fList = colFilled._list;

    list.append(Int(16,32));
    list.append(Int(32,64));
    list.traverseEmpty(colEmpty.f());
    list.traverseFilled(colFilled.f());

    REQUIRE( eList[0] == Int(0,16) );
    REQUIRE( eList[1] == Int(64,1024) );

    REQUIRE( fList[0] == Int(16,64) );
}

TEST_CASE("interval_list_append_shorten_front","[interval]") {
    typedef SafeLists::Interval Int;

    SafeLists::IntervalList list(Int(0,1024));
    IntervalCollector colEmpty;
    IntervalCollector colFilled;
    auto &eList = colEmpty._list;
    auto &fList = colFilled._list;

    list.append(Int(16,32));
    list.append(Int(48,64));
    list.append(Int(17,63));

    list.traverseEmpty(colEmpty.f());
    list.traverseFilled(colFilled.f());

    REQUIRE( eList[0] == Int(0,16) );
    REQUIRE( eList[1] == Int(64,1024) );

    REQUIRE( fList[0] == Int(16,64) );
}

TEST_CASE("interval_list_append_merge_all","[interval]") {
    typedef SafeLists::Interval Int;

    SafeLists::IntervalList list(Int(0,1024));
    IntervalCollector colEmpty;
    IntervalCollector colFilled;
    auto &eList = colEmpty._list;
    auto &fList = colFilled._list;

    list.append(Int(16,32));
    list.append(Int(48,64));
    list.append(Int(8,128));

    list.traverseEmpty(colEmpty.f());
    list.traverseFilled(colFilled.f());

    REQUIRE( eList[0] == Int(0,8) );
    REQUIRE( eList[1] == Int(128,1024) );

    REQUIRE( fList[0] == Int(8,128) );
}

TEST_CASE("interval_list_append_merge_inner","[interval]") {
    typedef SafeLists::Interval Int;

    SafeLists::IntervalList list(Int(0,1024));
    IntervalCollector colEmpty;
    IntervalCollector colFilled;
    auto &eList = colEmpty._list;
    auto &fList = colFilled._list;

    list.append(Int(16,32));
    list.append(Int(17,31));

    list.traverseEmpty(colEmpty.f());
    list.traverseFilled(colFilled.f());

    REQUIRE( eList[0] == Int(0,16) );
    REQUIRE( eList[1] == Int(32,1024) );

    REQUIRE( fList[0] == Int(16,32) );
}

TEST_CASE("interval_list_append_front","[interval]") {
    typedef SafeLists::Interval Int;

    SafeLists::IntervalList list(Int(0,1024));
    IntervalCollector colEmpty;
    IntervalCollector colFilled;
    auto &eList = colEmpty._list;
    auto &fList = colFilled._list;

    list.append(Int(24,64));
    list.append(Int(16,48));
    list.traverseEmpty(colEmpty.f());
    list.traverseFilled(colFilled.f());

    REQUIRE( eList[0] == Int(0,16) );
    REQUIRE( eList[1] == Int(64,1024) );

    REQUIRE( fList[0] == Int(16,64) );
}

TEST_CASE("interval_list_swallow_all","[interval]") {
    typedef SafeLists::Interval Int;

    SafeLists::IntervalList list(Int(0,1024));
    IntervalCollector colEmpty;
    IntervalCollector colFilled;
    auto &eList = colEmpty._list;
    auto &fList = colFilled._list;

    list.append(Int(16,32));
    list.append(Int(48,64));
    list.append(Int(0,1024));

    list.traverseEmpty(colEmpty.f());
    list.traverseFilled(colFilled.f());

    REQUIRE( SA::size(eList) == 0 );
    REQUIRE( fList[0] == Int(0,1024) );
}

TEST_CASE("interval_list_stress_a","[interval]") {
    typedef SafeLists::Interval Int;
    std::mt19937 generator(7);

    const int LIMIT = 256 * 256;
    SafeLists::IntervalList list(Int(0,LIMIT));

    TEMPLATIOUS_REPEAT( 10000 ) {
        int64_t current = generator() % LIMIT;
        int64_t end = current + 10;
        if (end > LIMIT) {
            end = LIMIT;
        }

        list.append(Int(current,end));
    }

    IntervalCollector colEmpty;
    IntervalCollector colFilled;
    auto &eList = colEmpty._list;
    auto &fList = colFilled._list;

    list.traverseEmpty(colEmpty.f());
    list.traverseFilled(colFilled.f());

    REQUIRE( SA::size(eList) == 2201 );
    REQUIRE( SA::size(fList) == 2200 );
}

TEST_CASE("interval_list_stress_b","[interval]") {
    typedef SafeLists::Interval Int;
    std::mt19937 generator(7);

    const int LIMIT = 256 * 16;
    SafeLists::IntervalList list(Int(0,LIMIT));

    TEMPLATIOUS_REPEAT( 18099 ) {
        int64_t current = generator() % LIMIT;
        int64_t end = current + 10;
        if (end > LIMIT) {
            end = LIMIT;
        }

        list.append(Int(current,end));
    }

    IntervalCollector colEmpty;
    IntervalCollector colFilled;
    auto &eList = colEmpty._list;
    auto &fList = colFilled._list;

    list.traverseEmpty(colEmpty.f());
    list.traverseFilled(colFilled.f());

    REQUIRE( SA::size(eList) == 0 );
    REQUIRE( SA::size(fList) == 1 );
}

TEST_CASE("interval_list_stress_c","[Interval]") {
    typedef SafeLists::Interval Int;
    std::mt19937 generator(777);

    const int LIMIT = 256 * 256;
    SafeLists::IntervalList list(Int(0,LIMIT));

    TEMPLATIOUS_REPEAT( 10000 ) {
        int64_t current = generator() % LIMIT;
        int64_t switcher = generator() % 10;
        int64_t end = current + 10;
        if (switcher == 9) {
            end = current + 1000;
        } else if (switcher > 6) {
            end = current + 100;
        }

        list.append(Int(current,end));
    }

    IntervalCollector colEmpty;
    IntervalCollector colFilled;
    auto &eList = colEmpty._list;
    auto &fList = colFilled._list;

    list.traverseEmpty(colEmpty.f());
    list.traverseFilled(colFilled.f());

    REQUIRE( SA::size(eList) == 0 );
    REQUIRE( SA::size(fList) == 1 );
}
