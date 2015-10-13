
#include <templatious/FullPack.hpp>

#include "catch.hpp"

#include <util/DumbHash.hpp>
#include <util/ScopeGuard.hpp>
#include <util/GenericStMessageable.hpp>

TEMPLATIOUS_TRIPLET_STD;

TEST_CASE("dumb_hash_basic","[util]") {
    SafeLists::DumbHash256 hash;
    char buf[65];
    hash.toString(buf);

    std::string expected =
        "37373737"
        "37373737"
        "37373737"
        "37373737"
        "37373737"
        "37373737"
        "37373737"
        "37373737";

    REQUIRE( buf == expected );
}

TEST_CASE("dumb_hash_basic_append_end","[util]") {
    SafeLists::DumbHash256 hash;

    hash.add(249,255);

    char buf[65];
    hash.toString(buf);

    std::string expected =
        "b7373737"
        "37373737"
        "37373737"
        "37373737"
        "37373737"
        "37373737"
        "37373737"
        "373737c8";

    REQUIRE( buf == expected );
}

TEST_CASE("dumb_hash_basic_append_single","[util]") {
    SafeLists::DumbHash256 hash;

    hash.add(16,255);

    char buf[65];
    hash.toString(buf);

    std::string expected =
        "3737c837"
        "37373737"
        "37373737"
        "37373737"
        "37373737"
        "37373737"
        "37373737"
        "37373737";

    REQUIRE( buf == expected );
}

TEST_CASE("dumb_hash_basic_append_midsplit","[util]") {
    SafeLists::DumbHash256 hash;

    hash.add(3,255);

    char buf[65];
    hash.toString(buf);

    std::string expected =
        "c8d73737"
        "37373737"
        "37373737"
        "37373737"
        "37373737"
        "37373737"
        "37373737"
        "37373737";

    REQUIRE( buf == expected );
}

TEST_CASE("dumb_hash_the_cake","[util]") {
    std::mt19937 tw(7);
    std::vector< char > toApply;
    TEMPLATIOUS_REPEAT( 256 * 2 ) {
        SA::add(toApply,tw());
    }

    SafeLists::DumbHash256 hashA;
    SafeLists::DumbHash256 hashB;
    auto sf = SF::seqL(256*2);
    TEMPLATIOUS_FOREACH(auto i,sf) {
        hashA.add(i,toApply[i]);
    }

    auto sr = sf.rev();
    TEMPLATIOUS_FOREACH(auto i,sr) {
        hashB.add(i,toApply[i]);
    }

    char strA[65];
    char strB[65];
    hashA.toString(strA);
    hashB.toString(strB);

    std::string aStr = strA;
    std::string bStr = strB;
    REQUIRE( aStr == bStr );
}

TEST_CASE("dumb_hash_raw_dump","[util]") {
    SafeLists::DumbHash256 hash;

    hash.add(3,255);

    char buf[32];
    hash.toBytes(buf);

    std::string expected =
        "c8d73737"
        "37373737"
        "37373737"
        "37373737"
        "37373737"
        "37373737"
        "37373737"
        "37373737";

    REQUIRE( buf[0] == char(0xc8) );
    REQUIRE( buf[1] == char(0xd7) );
    bool doesMatch = true;
    auto seq = SF::seqL(2,32);
    TEMPLATIOUS_FOREACH(auto i,seq) {
        doesMatch &= buf[i] == char(0x37);
    }

    REQUIRE( doesMatch );

    SafeLists::DumbHash256 hash2;
    hash2.setBytes(buf);

    char outStr[65];
    hash2.toString(outStr);
    REQUIRE( outStr == expected );
}

TEST_CASE("scope_guard_simple","[util]") {
    bool called = false;
    {
        auto g = SafeLists::makeScopeGuard(
            [&]() {
                called = true;
            }
        );
    }

    REQUIRE( called );
}

TEST_CASE("scope_guard_dismiss","[util]") {
    bool called = false;
    {
        auto g = SafeLists::makeScopeGuard(
            [&]() {
                called = true;
            }
        );
        g.dismiss();
    }

    REQUIRE( !called );
}

TEST_CASE("scope_guard_fire","[util]") {
    int count = 0;
    {
        auto g = SCOPE_GUARD_LC(
            ++count;
        );

        REQUIRE( count == 0 );

        g.fire();

        REQUIRE( count == 1 );
    }
    REQUIRE( count == 1 );
}

struct MessA : public SafeLists::GenericStMessageable {
    MessA() {
        regHandler(
            SF::virtualMatchFunctorPtr(
                SF::virtualMatch< int >(
                    [](int& i) {
                        i = 7;
                    }
                ),
                SF::virtualMatch< short >(
                    [](short& i) {
                        i = 77;
                    }
                )
            )
        );
    }
};

struct MessB : public MessA {
    MessB() {
        regHandler(
            SF::virtualMatchFunctorPtr(
                SF::virtualMatch< int >(
                    [](int& i) {
                        i = 777;
                    }
                )
            )
        );
    }
};

TEST_CASE("generic_messageable_inheritance","[util]") {
    MessA a;
    MessB b;

    auto intMsg = SF::vpack< int >(0);
    auto shortMsg = SF::vpack< short >(0);

    a.message(intMsg);
    a.message(shortMsg);

    REQUIRE( intMsg.fGet<0>() == 7 );
    REQUIRE( shortMsg.fGet<0>() == 77 );

    intMsg.fGet<0>() = 0;
    shortMsg.fGet<0>() = 0;

    b.message(intMsg);
    b.message(shortMsg);

    REQUIRE( intMsg.fGet<0>() == 777 );
    REQUIRE( shortMsg.fGet<0>() == 77 );
}
