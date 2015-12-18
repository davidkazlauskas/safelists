
#include <fstream>
#include <cstring>

#include <templatious/FullPack.hpp>
#include <io/RandomFileWriterImpl.cpp>
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

TEST_CASE("io_dynamic_read_write_create","[io]") {
    FileEraser er;
    SA::add(er._files,"a.txt");
    const char* str = "dovahkiin";

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

TEST_CASE("io_actor_random_file_writer","[io]") {
    using namespace SafeLists;
    FileEraser er;
    SA::add(er._files,"a.txt");
    const char* str = "dovahkiin";

    auto writer = RandomFileWriter::make();

    typedef std::unique_ptr< char[] > BufPtr;
    BufPtr buffer( new char[strlen(str)] );
    memcpy(buffer.get(),str,strlen(str));

    auto writeMessage = SF::vpackPtr<
        RandomFileWriter::WriteData,
        std::string,
        BufPtr,
        int64_t,
        int64_t
    >(
        nullptr,
        "a.txt",
        std::move(buffer),
        0,
        strlen(str)
    );
    writer->message(writeMessage);
    auto closeMessage = SF::vpackPtrCustom<
        templatious::VPACK_WAIT,
        RandomFileWriter::ClearCache
    >(nullptr);
    writer->message(closeMessage);
    closeMessage->wait();

    RandomFileWriteHandle reader("a.txt",-1);
    char buf[64];
    reader.read(buf,0,strlen(str));
    buf[strlen(str)] = '\0';

    REQUIRE( 0 == strcmp(str,buf) );
}

TEST_CASE("io_simple_read_write_create_dir","[io]") {
    const char* inDirFile = "somedir/moarstuff/a.txt";
    std::remove( inDirFile );

    SafeLists::RandomFileWriteHandle handle(inDirFile,7);
    handle.write("thetext",0,7);
    char output[8];
    handle.read(output,0,7);
    output[7] = '\0';

    std::string comp = output;
    REQUIRE( comp == "thetext" );

    REQUIRE( handle.getPath() == inDirFile );

    std::remove( inDirFile );
}

TEST_CASE("io_file_exists","[io]") {
    using namespace SafeLists;
    FileEraser er;
    SA::add(er._files,"a.txt");

    {
        std::ofstream a("a.txt");
        a << "moo";
    }

    auto writer = RandomFileWriter::make();

    auto checkNoExist = SF::vpackPtrCustom<
        templatious::VPACK_WAIT,
        RandomFileWriter::DoesFileExist,
        std::string,
        bool
    >(nullptr,"az.txt",true);
    auto checkExists = SF::vpackPtrCustom<
        templatious::VPACK_WAIT,
        RandomFileWriter::DoesFileExist,
        std::string,
        bool
    >(nullptr,"a.txt",false);


    writer->message(checkNoExist);
    writer->message(checkExists);

    checkNoExist->wait();
    checkExists->wait();

    bool a = checkNoExist->fGet<2>();
    bool b = checkExists->fGet<2>();

    REQUIRE( false == a );
    REQUIRE( true == b );
}

TEST_CASE("io_file_delete","[io]") {
    using namespace SafeLists;
    FileEraser er;
    SA::add(er._files,"a.txt");

    {
        std::ofstream a("a.txt");
        a << "moo";
    }

    auto writer = RandomFileWriter::make();

    auto deleteFile = SF::vpackPtrCustom<
        templatious::VPACK_WAIT,
        RandomFileWriter::DeleteFile,
        std::string
    >(nullptr,"a.txt");

    writer->message(deleteFile);
    deleteFile->wait();

    bool exists = boost::filesystem::exists("a.txt");
    REQUIRE( false == exists );
}
