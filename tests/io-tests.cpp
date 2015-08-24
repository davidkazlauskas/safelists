
#include <templatious/FullPack.hpp>
#include <io/RandomFileWriter.hpp>

#include "catch.hpp"

TEMPLATIOUS_TRIPLET_STD;

TEST_CASE("io_simple_read_write","[io]") {
    SafeLists::RandomFileWriteHandle handle("some_file.txt",7);
    handle.write("thetext",0,7);
    char output[8];
    handle.read(output,0,7);
    output[7] = '\0';

    std::string comp = output;
    REQUIRE( comp == "thetext" );
}
