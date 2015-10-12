
#include <templatious/FullPack.hpp>
#include <boost/filesystem.hpp>

#include <meta/SignatureCheck.hpp>
#include <util/ScopeGuard.hpp>

#include "catch.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace fs = boost::filesystem;

TEST_CASE("sha_256_many_files","[meta]") {
    std::vector< std::string > files;
    SA::add(files,"a.txt","b.txt");
    auto sg = SCOPE_GUARD_LC(
        TEMPLATIOUS_FOREACH(auto& i,files) {
            fs::remove(i);
        }
    );

    fs::copy_file("exampleData/full-schema.sql","a.txt");
    fs::copy_file("exampleData/trigger-drop.sql","b.txt");

    auto res = SafeLists::hashFileListSha256(files);

    REQUIRE( res == "" );
}
