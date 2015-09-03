
#include <fstream>
#include <cstring>

#include <templatious/FullPack.hpp>
#include <io/RandomFileWriter.hpp>

#include "catch.hpp"

TEMPLATIOUS_TRIPLET_STD;

#define VICTIM_NAME "some_file.txt"

TEST_CASE("io_simple_read_write","[io]") {
    std::remove( VICTIM_NAME );

    SafeLists::RandomFileWriteHandle handle(VICTIM_NAME,7);
    handle.write("thetext",0,7);
    char output[8];
    handle.read(output,0,7);
    output[7] = '\0';

    std::string comp = output;
    REQUIRE( comp == "thetext" );

    REQUIRE( handle.getPath() == VICTIM_NAME );

    std::remove( VICTIM_NAME );
}

TEST_CASE("io_range_throw_diff_size","[io]") {
    std::remove( VICTIM_NAME );

    {
        std::ofstream file(VICTIM_NAME);
        file << "abc";
    }

    bool caught = false;
    try {
        SafeLists::RandomFileWriteHandle handle(VICTIM_NAME,7);
    } catch (const SafeLists::RandomFileWriterDifferentFileSizeException&) {
        caught = true;
    }

    REQUIRE( caught );

    std::remove( VICTIM_NAME );
}

TEST_CASE("io_range_throw_bad_write","[io]") {
    std::remove( VICTIM_NAME );

    SafeLists::RandomFileWriteHandle handle(VICTIM_NAME,7);

    {
        bool caught = false;
        try {
            const char* txt = "cow say moo";
            handle.write(txt,0,strlen(txt));
        } catch (const SafeLists::RandomFileWriterOutOfBoundsWriteException&) {
            caught = true;
        }

        REQUIRE( caught );
    }

    {
        bool caught = false;
        try {
            const char* txt = "cow";
            handle.write(txt,-1,strlen(txt));
        } catch (const SafeLists::RandomFileWriterOutOfBoundsWriteException&) {
            caught = true;
        }

        REQUIRE( caught );
    }

    std::remove( VICTIM_NAME );
}

struct FileEraser {
    ~FileEraser() {
        TEMPLATIOUS_FOREACH(auto& i,_files) {
            std::remove(i.c_str());
        }
    }

    std::vector< std::string > _files;
};

TEST_CASE("io_random_writer_cache_basic","[io]") {
    FileEraser er;
    SA::add(er._files,"a.txt","b.txt","c.txt");

    SafeLists::RandomFileWriteCache cache(2);

    REQUIRE( !cache.isCached("a.txt") );
    auto item = cache.getItem("a.txt",16);
    REQUIRE( cache.isCached("a.txt") );
    auto item2 = cache.getItem("a.txt",16);
    REQUIRE( item == item2 );
}

TEST_CASE("io_random_writer_cache_overwrite","[io]") {
    FileEraser er;
    SA::add(er._files,"a.txt","b.txt","c.txt");

    SafeLists::RandomFileWriteCache cache(2);

    auto itemA = cache.getItem("a.txt",16);
    auto itemB = cache.getItem("b.txt",16);

    REQUIRE(  cache.isCached("a.txt") );
    REQUIRE(  cache.isCached("b.txt") );
    REQUIRE( !cache.isCached("c.txt") );

    auto itemC = cache.getItem("c.txt",16);
    REQUIRE( !cache.isCached("a.txt") );
    REQUIRE(  cache.isCached("b.txt") );
    REQUIRE(  cache.isCached("c.txt") );
}

TEST_CASE("io_random_writer_cache_heat","[io]") {
    FileEraser er;
    SA::add(er._files,"a.txt","b.txt","c.txt");

    SafeLists::RandomFileWriteCache cache(2);

    auto itemA = cache.getItem("a.txt",16);
    auto itemB = cache.getItem("b.txt",16);
    auto itemAA = cache.getItem("a.txt",16);
    // a.txt should be hotter in cache than b
    // because it was asked latest

    REQUIRE(  cache.isCached("a.txt") );
    REQUIRE(  cache.isCached("b.txt") );
    REQUIRE( !cache.isCached("c.txt") );

    // b should get destroyed
    auto itemC = cache.getItem("c.txt",16);
    REQUIRE(  cache.isCached("a.txt") );
    REQUIRE( !cache.isCached("b.txt") );
    REQUIRE(  cache.isCached("c.txt") );
}

TEST_CASE("io_dynamic_read_write","[io]") {
    FileEraser er;
    SA::add(er._files,"a.txt");
    const char* str = "dovahkiin";

    {
        std::ofstream out("a.txt");
        out.write("7",1);
    }

    {
        SafeLists::RandomFileWriteHandle handle("a.txt",-1);
        handle.write(str,77,strlen(str));
    }

    {
        SafeLists::RandomFileWriteHandle handle("a.txt",-1);
        char arr[64];
        handle.read(arr,77,strlen(str));
        arr[strlen(str)] = '\0';

        REQUIRE( 0 == strcmp(arr,str) );
    }
}
