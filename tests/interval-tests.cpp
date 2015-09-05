
#include <random>
#include <fstream>

#include <templatious/FullPack.hpp>
#include <io/Interval.hpp>
#include <util/Misc.hpp>

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

TEST_CASE("interval_size","[interval]") {
    typedef SafeLists::Interval Int;
    REQUIRE( Int(0,10).size() == 10 );
    REQUIRE( Int(-10,10).size() == 20 );
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

    REQUIRE( SA::size(eList) == 2037 );
    REQUIRE( SA::size(fList) == 2036 );
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

TEST_CASE("interval_list_stress_c","[interval]") {
    typedef SafeLists::Interval Int;
    std::mt19937 generator(777);

    const int LIMIT = 256 * 256;
    SafeLists::IntervalList list(Int(0,LIMIT));

    TEMPLATIOUS_REPEAT( 77236 ) {
        int64_t current = generator() % LIMIT;
        int64_t switcher = generator() % 10;
        int64_t end = current + 10;

        if (switcher == 9) {
            end = current + 1000;
        } else if (switcher > 6) {
            end = current + 100;
        }

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

#ifdef SAFELISTS_TESTING
TEST_CASE("interval_list_stress_d","[interval]") {
    typedef SafeLists::Interval Int;
    std::mt19937 generator(7);

    const int LIMIT[] = { 256, 256 * 16, 256 * 256, 256 * 256 * 16 };

    int integrityFail = -1;
    TEMPLATIOUS_REPEAT( 1000 ) {
        std::mt19937 generatorInner(generator());
        auto specLimit = LIMIT[generator() % (sizeof(LIMIT) / sizeof(LIMIT[0]))];
        SafeLists::IntervalList list(Int(0,specLimit));

        TEMPLATIOUS_REPEAT( 1000 ) {
            int64_t current = generatorInner() % specLimit;
            int64_t switcher = generator() % 10;
            int64_t end = current + 10;

            if (switcher == 9) {
                end = current + 1000;
            } else if (switcher > 6) {
                end = current + 100;
            }

            auto toCheck = Int(current,end);
            if (list.doesBelong(toCheck)) {
                list.append(toCheck);
            }
        }

        bool isGood = list.checkIntegrity();
        if (!isGood) {
            integrityFail = __tmp_i;
            break;
        }
    }

    REQUIRE( -1 == integrityFail );
}
#endif

TEST_CASE("interval_list_throw_foreign","[interval]") {
    typedef SafeLists::Interval Int;
    SafeLists::IntervalList list(Int(0,128));

    {
        bool caught = false;
        try {
            list.append(Int(-1,0));
        } catch (const SafeLists::IntervalListIntervalDoesntBelongException&) {
            caught = true;
        }
        REQUIRE( caught );
    }

    {
        bool caught = false;
        try {
            list.append(Int(-2,-1));
        } catch (const SafeLists::IntervalListIntervalDoesntBelongException&) {
            caught = true;
        }
        REQUIRE( caught );
    }

    {
        bool caught = false;
        try {
            list.append(Int(128,129));
        } catch (const SafeLists::IntervalListIntervalDoesntBelongException&) {
            caught = true;
        }
        REQUIRE( caught );
    }

    {
        bool caught = false;
        try {
            list.append(Int(-1,129));
        } catch (const SafeLists::IntervalListIntervalDoesntBelongException&) {
            caught = true;
        }
        REQUIRE( caught );
    }
}

struct IntervalCounter {
    IntervalCounter() : _size(0) {}

    std::function<bool(const SafeLists::Interval&)> f() {
        return [=](const SafeLists::Interval& i) {
            _size += i.size();
            return true;
        };
    }

    int64_t _size;
};

#ifdef SAFELISTS_TESTING
TEST_CASE("interval_list_random_intervals_stress","[interval]") {
    typedef SafeLists::Interval Int;
    std::mt19937 generator(7);

    const int LIMIT[] = { 256, 256 * 16, 256 * 256 };
    const int SHIP_LIMIT[] = {16,128,256,17,73,777,1024,4096,7777};
    int integrityFail = -1;
    TEMPLATIOUS_REPEAT( 1000 ) {
        std::mt19937 generatorInner(generator());
        auto specLimit = LIMIT[generator() % (sizeof(LIMIT) / sizeof(LIMIT[0]))];
        SafeLists::IntervalList list(Int(0,specLimit));
        auto remainingSize = specLimit;

        while (remainingSize > 0 && integrityFail == -1) {
            auto specShipSize = SHIP_LIMIT[generator() % (sizeof(SHIP_LIMIT) / sizeof(SHIP_LIMIT[0]))];
            list.randomEmptyIntervals(specShipSize,
                [&](const Int& i) {

                    remainingSize -= i.size();
                    IntervalCounter cntPre;
                    list.traverseEmpty(cntPre.f());
                    list.append(i);

                    IntervalCounter cnt;
                    list.traverseEmpty(cnt.f());

                    if (!list.checkIntegrity() || remainingSize != cnt._size) {
                        integrityFail = 10000 * __tmp_i;
                        return false;
                    }

                    return true;
                }
            );
        }

        if (integrityFail > 0) {
            integrityFail += __tmp_i;
            break;
        }
    }

    REQUIRE( -1 == integrityFail );
}
#endif

TEST_CASE("endianess_check","[misc]") {
    const int64_t theNumber = 1234567890;
    char arr[sizeof(theNumber)];
    SafeLists::writeI64AsLittleEndian(theNumber,arr);
    int64_t outNum = 0;
    SafeLists::readI64FromLittleEndian(outNum,arr);

    REQUIRE( theNumber == outNum );
}

TEST_CASE("interval_list_equality_check","[interval]") {
    using namespace SafeLists;

    IntervalList list(Interval(0,512));
    list.append(Interval(1,7));
    list.append(Interval(77,128));

    auto clone = list.clone();
    REQUIRE( areIntervalListsEqual(list,clone) );

    list.append(Interval(7,10));
    REQUIRE( !areIntervalListsEqual(list,clone) );
    bool another = areIntervalListsEqual(list,IntervalList(Interval(0,511)));
    REQUIRE( !another );
    clone.append(Interval(7,10));
    REQUIRE( areIntervalListsEqual(list,clone) );
    clone.append(Interval(300,310));
    REQUIRE( !areIntervalListsEqual(list,clone) );
}

TEST_CASE("interval_list_write_read_test","[interval]") {
    using namespace SafeLists;

    IntervalList list(Interval(0,512));
    list.append(Interval(1,7));
    list.append(Interval(77,128));

    {
        std::ofstream os( "tmp.bin", std::ios::binary );
        writeIntervalList(list,os);
    }


    auto out = []() {
        std::ifstream is( "tmp.bin", std::ios::binary );
        return readIntervalList(is);
    }();

    REQUIRE( areIntervalListsEqual(list,out) );
}

TEST_CASE("interval_list_is_defined","[interval]") {
    using namespace SafeLists;

    Interval empty;
    IntervalList listE(empty);

    REQUIRE( !listE.isDefined() );

    IntervalList listV(Interval(0,7));

    REQUIRE( listV.isDefined() );
}
