
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
      "138016af591e7b87798457d1eb60423c55de96438ecee23107c38b448b9a608782075cf5d64"
      "520b47f74b0e820badf985a5b736815da208dc865e149803d94a97484e525d5417387500460"
      "e67f36232c91da32a1ab3c83390b4508290344ee062c11dda1b8d76b7f0128f8e79ec2743e2"
      "789290d80dc25ad7b613acd2a8e35a93e8832a69ded43484176fe95e6561431a9a54c86d48f"
      "4cb8a7e456c1d4149002baef199b461efab8d10a0bb7503a909973ca0363a74bf7bdf88a37b"
      "720a0e2f4c1cf7989cc3dd858e9aa9c961b0de8410732deef6c67bfc3100110643ca5aeab05"
      "5ca3b10f1e43b11adb12bdb73216f92199826da6b3a22777b0da873b537f3a";

    REQUIRE( outSig == EXPECTED );
}

