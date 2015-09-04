
#include <templatious/FullPack.hpp>

#include "catch.hpp"

#include <util/DumbHash.hpp>

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
