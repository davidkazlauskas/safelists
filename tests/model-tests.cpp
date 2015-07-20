
#define CATCH_CONFIG_MAIN
#include <templatious/FullPack.hpp>

#include "catch.hpp"

#include <model/IndexedListView.hpp>
#include <model/AsyncSqlite.hpp>

TEMPLATIOUS_TRIPLET_STD;

using namespace SafeLists;

TEST_CASE("model_string_snapshot","[model]") {
}

TEST_CASE("model_basic_sqlite","[model]") {
    auto msg = AsyncSqlite::createNew(":memory:");

    {
        auto pack = SF::vpackPtrCustom< templatious::VPACK_WAIT,
             AsyncSqlite::Execute, const char*
        >(
            nullptr, ""
        );
    }
}

