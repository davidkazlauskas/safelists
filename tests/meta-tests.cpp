
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

    auto res = SafeLists::hashFileListSha256(files);

    REQUIRE( res ==
            "2dd1f026127f394924d48f3c44e8044c"
            "b64739d2c268053e0907037006a818ed" );
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

    std::string outSig;
    auto res = SafeLists::signFileList("exampleData/rsa-keys/private-test.pem",files,outSig);
    REQUIRE( res == SafeLists::SignFileListError::Success );
    REQUIRE( outSig != "" );

    auto resVer = SafeLists::verifyFileList(
        "exampleData/rsa-keys/public-test.pem",outSig.c_str(),files);

    REQUIRE( resVer == SafeLists::VerifyFileListError::Success );

    REQUIRE( outSig ==
        "6153f401ba6bfecbff926152abf39545a2f0d987f4483673de30fcb94eb5d5807c8acd61cdc"
        "0c5b733a17dec7fc6d48818772890d35ea29c60ff7aa5c92203aff9f45b7d32cd840d9cec1d"
        "428f96f4aebc96eb5f723f99e3f1ae20f9b6c80e5a6667bc67b0e1e143ec18db6d5111cd3c2"
        "622b2cfee96fe6323618a657d1d130d882e90a6bc0ecbf22cf46ea73074d1299e697e8b4ad8"
        "47512080564f123a363b63ea563fe1ab1414a05659873386a1a1ead11abc0cea96b06f445f1"
        "46ec0254df7bd7e904b043eaa9e184a991767127007ad3af6d8bbde9912218a44c61df21e90"
        "74a4585237fe1f243db6f905782b0c008b62528d5f682b6b50f0a12f29171c" );
}
