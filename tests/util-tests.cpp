
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
