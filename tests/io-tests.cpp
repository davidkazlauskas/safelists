
#include <templatious/FullPack.hpp>
#include <io/RandomFileWriter.hpp>

#include "catch.hpp"

TEMPLATIOUS_TRIPLET_STD;

#define VICTIM_NAME "some_file.txt"

TEST_CASE("io_simple_read_write","[io]") {
    SafeLists::RandomFileWriteHandle handle(VICTIM_NAME,7);
    handle.write("thetext",0,7);
    char output[8];
    handle.read(output,0,7);
    output[7] = '\0';

    std::string comp = output;
    REQUIRE( comp == "thetext" );

    std::remove( VICTIM_NAME );
}

TEST_CASE("io_range_throw","[io]") {

}
