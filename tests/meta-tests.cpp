
#include <templatious/FullPack.hpp>
#include <boost/filesystem.hpp>

#include <meta/SignatureCheck.hpp>
#include <util/ScopeGuard.hpp>

#include "catch.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace fs = boost::filesystem;

TEST_CASE("sha_256_many_files","[meta]") {
    std::vector< std::string > files;
    SA::add(files,
        "a.txt",
        "b.txt"
    );
    auto sg = SCOPE_GUARD_LC(
        TEMPLATIOUS_FOREACH(auto& i,files) {
            fs::remove(i);
        }
    );

    fs::copy_file("exampleData/full-schema.sql","a.txt");
    fs::copy_file("exampleData/trigger-drop.sql","b.txt");

    std::string rootPath = "./";
    auto res = SafeLists::hashFileListSha256(rootPath,files);

    const char* EXPECTED =
        "42f640590efd829a8f360a1f6dd3ee10"
        "3cfb366fae5f2c2c8a4653e15f4a63ee";
    REQUIRE( res == EXPECTED );
}

TEST_CASE("sign_files","[meta]") {
    std::vector< std::string > files;
    SA::add(files,
        "a.txt",
        "b.txt"
    );
    auto sg = SCOPE_GUARD_LC(
        TEMPLATIOUS_FOREACH(auto& i,files) {
            fs::remove(i);
        }
    );

    fs::copy_file("exampleData/full-schema.sql","a.txt");
    fs::copy_file("exampleData/trigger-drop.sql","b.txt");

    std::string rootPath = "./";
    std::string outSig;
    auto res = SafeLists::signFileList(
        "exampleData/rsa-keys/examplekeys.json",rootPath,files,outSig);
    REQUIRE( res == SafeLists::SignFileListError::Success );
    REQUIRE( outSig != "" );

    auto resVer = SafeLists::verifyFileList(
        "exampleData/rsa-keys/examplekeys.json",outSig.c_str(),rootPath,files);

    REQUIRE( resVer == SafeLists::VerifyFileListError::Success );

    const char* EXPECTED =
      "rdqX9koea76o20SOsMdMhPpaLktftYANBgazIFQ4amvy"
      "zTcQMOhciVuoEjhGEhqAfmi2QvTA66uGjEMAjXpMAw==";

    REQUIRE( outSig == EXPECTED );
}

