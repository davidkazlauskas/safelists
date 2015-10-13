
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
            "" );
}
