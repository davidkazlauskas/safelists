
#include <templatious/FullPack.hpp>
#include <io/AsyncDownloader.hpp>

#include "catch.hpp"

TEMPLATIOUS_TRIPLET_STD;

namespace {
    bool ensureExpected(int64_t size,const char* buffer) {
        TEMPLATIOUS_0_TO_N(i,size) {
            if (buffer[i] != '7') {
                return false;
            }
        }
        return true;
    }
}

TEST_CASE("async_downloader_dummy","[async_downloader]") {
    auto downloader = SafeLists::AsyncDownloader::createNew("imitation");

}

